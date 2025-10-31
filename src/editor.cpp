#include "editor.h"
#include <iostream>
#include <sstream>

namespace subzero {

Editor::Editor(shared_ptr<ITerminal> terminal)
    : m_terminal(terminal)
    , m_buffer(shared_ptr<Buffer>(new Buffer()))
    , m_current_buffer_index(0)
    , m_mode(NORMAL)
    , m_previous_mode(NORMAL)
    , m_search_forward(true)
    , m_running(false)
    , m_dirty_display(true)
    , m_yank_line_mode(false)
    , m_repeat_count(0)
    , m_syntax_manager(new SyntaxHighlighterManager())
{
    if (m_terminal) {
        // Initialize with one empty buffer
        m_buffers.push_back(m_buffer);
        m_window = shared_ptr<Window>(new Window(m_terminal, m_buffer));
        initializeKeyBindings();
        
        // Syntax highlighting is now built-in, no plugin loading needed
    }
}

Editor::~Editor() {
    delete m_syntax_manager;
}

void Editor::run() {
    if (!m_terminal || !m_terminal->initialize()) {
        return;
    }
    
    m_running = true;
    m_terminal->clear();
    
    // Setup window to use full terminal except status bar
    TerminalSize terminal_size = m_terminal->getSize();
    m_window->setPosition(Position(0, 0));
    m_window->setSize(TerminalSize(terminal_size.rows - 1, terminal_size.cols));
    
    while (m_running) {
        if (m_dirty_display) {
            render();
            m_dirty_display = false;
        }
        
        handleInput();
    }
    
    m_terminal->shutdown();
}

bool Editor::openFile(const std::string& filename) {
    shared_ptr<Buffer> new_buffer(new Buffer());
    
    // Try to load the file
    bool file_loaded = new_buffer->loadFromFile(filename);
    
    // If file doesn't exist, create a new buffer with the filename
    if (!file_loaded) {
        new_buffer->setFilename(filename);
    }
    
    // Add to buffer list and switch to it
    m_buffers.push_back(new_buffer);
    m_current_buffer_index = m_buffers.size() - 1;
    m_buffer = new_buffer;
    m_window->setBuffer(m_buffer);
    
    // Set up syntax highlighting for this file
    if (m_syntax_manager) {
        ISyntaxHighlighter* highlighter = m_syntax_manager->getHighlighterForFile(filename);
        m_window->setSyntaxHighlighter(highlighter);
        if (file_loaded) {
            if (highlighter) {
                setStatusMessage("Opened: " + filename + " (" + highlighter->getName() + ") [Buffer " + 
                               compat::to_string(m_current_buffer_index + 1) + "]");
            } else {
                setStatusMessage("Opened: " + filename + " [Buffer " + compat::to_string(m_current_buffer_index + 1) + "]");
            }
        } else {
            if (highlighter) {
                setStatusMessage("New file: " + filename + " (" + highlighter->getName() + ") [Buffer " + 
                               compat::to_string(m_current_buffer_index + 1) + "]");
            } else {
                setStatusMessage("New file: " + filename + " [Buffer " + compat::to_string(m_current_buffer_index + 1) + "]");
            }
        }
    } else {
        if (file_loaded) {
            setStatusMessage("Opened: " + filename + " [Buffer " + compat::to_string(m_current_buffer_index + 1) + "]");
        } else {
            setStatusMessage("New file: " + filename + " [Buffer " + compat::to_string(m_current_buffer_index + 1) + "]");
        }
    }
    
    m_dirty_display = true;
    return true;
}

bool Editor::saveFile(const std::string& filename) {
    if (m_buffer->saveToFile(filename)) {
        setStatusMessage("Saved: " + (filename.empty() ? m_buffer->getFilename() : filename));
        m_dirty_display = true;
        return true;
    }
    
    setErrorMessage("Could not save file");
    return false;
}

bool Editor::newFile() {
    m_buffer = shared_ptr<Buffer>(new Buffer());
    m_buffer->setCursor(BufferPosition(0, 0));
    
    m_window->setBuffer(m_buffer);
    setStatusMessage("New file");
    m_dirty_display = true;
    return true;
}

void Editor::setMode(EditorMode mode) {
    m_previous_mode = m_mode;
    m_mode = mode;
    m_dirty_display = true;
    
    if (mode == INSERT) {
        m_terminal->showCursor(true);
    } else {
        m_terminal->showCursor(true); // Keep cursor visible in all modes for now
    }
}

std::string Editor::getModeString() const {
    switch (m_mode) {
        case NORMAL: return "NORMAL";
        case INSERT: return "INSERT";
        case VISUAL: return "VISUAL";
        case VISUAL_LINE: return "VISUAL LINE";
        case COMMAND: return "COMMAND";
        case SEARCH: return "SEARCH";
        default: return "UNKNOWN";
    }
}

void Editor::render() {
    if (!m_terminal) return;
    
    // Ensure syntax highlighter is set correctly
    if (m_syntax_manager && m_buffer && !m_buffer->getFilename().empty()) {
        ISyntaxHighlighter* highlighter = m_syntax_manager->getHighlighterForFile(m_buffer->getFilename());
        m_window->setSyntaxHighlighter(highlighter);
    }
    
    // Render main window
    m_window->render();
    
    // Render status bar
    renderStatusBar();
    
    m_terminal->refresh();
    
    // Update cursor position after everything is rendered
    m_window->updateCursor();
}

void Editor::renderStatusBar() {
    if (!m_terminal) return;
    
    TerminalSize terminal_size = m_terminal->getSize();
    Position status_pos(terminal_size.rows - 1, 0);
    
    // Clear status line
    std::string status_line(terminal_size.cols, ' ');
    m_terminal->putStringWithColor(status_line, status_pos, Color::WHITE, Color::BLUE);
    
    // Build status string
    std::ostringstream status;
    status << " " << getModeString();
    
    if (!m_buffer->getFilename().empty()) {
        status << " | " << m_buffer->getFilename();
    } else {
        status << " | [No Name]";
    }
    
    if (m_buffer->isModified()) {
        status << " [+]";
    }
    
    // Cursor position
    const BufferPosition& cursor = m_buffer->getCursor();
    status << " | " << (cursor.line + 1) << ":" << (cursor.column + 1);
    
    // Total lines
    status << " | " << m_buffer->getLineCount() << " lines";
    
    // Show messages
    if (!m_error_message.empty()) {
        status.str("");
        status << " ERROR: " << m_error_message;
    } else if (!m_status_message.empty()) {
        status.str("");
        status << " " << m_status_message;
    }
    
    // Command line mode
    if (m_mode == COMMAND) {
        status.str("");
        status << ":" << m_command_line;
    } else if (m_mode == SEARCH) {
        status.str("");
        status << (m_search_forward ? "/" : "?") << m_command_line;
    }
    
    // Show command sequence if building one
    if (!m_command_sequence.empty()) {
        status.str("");
        status << " " << getModeString() << " | " << m_command_sequence;
        if (m_repeat_count > 0) {
            status.str("");
            status << " " << getModeString() << " | " << m_repeat_count << m_command_sequence;
        }
    } else if (m_repeat_count > 0) {
        status.str("");
        status << " " << getModeString() << " | " << m_repeat_count;
    }
    
    std::string status_text = status.str();
    if (status_text.length() > static_cast<size_t>(terminal_size.cols)) {
        status_text = status_text.substr(0, terminal_size.cols);
    }
    
    m_terminal->putStringWithColor(status_text, status_pos, Color::WHITE, Color::BLUE);
}

void Editor::setStatusMessage(const std::string& message) {
    m_status_message = message;
    m_error_message.clear();
    m_dirty_display = true;
}

void Editor::setErrorMessage(const std::string& message) {
    m_error_message = message;
    m_status_message.clear();
    m_dirty_display = true;
}

void Editor::handleInput() {
    if (!m_terminal) return;
    
    KeyPress key = m_terminal->getKey();
    
    // Clear messages after input
    clearMessages();
    
    switch (m_mode) {
        case NORMAL:
            handleNormalMode(key);
            break;
        case INSERT:
            handleInsertMode(key);
            break;
        case VISUAL:
        case VISUAL_LINE:
            handleVisualMode(key);
            break;
        case COMMAND:
            handleCommandMode(key);
            break;
        case SEARCH:
            handleSearchMode(key);
            break;
    }
    
    // Ensure cursor is visible
    m_window->ensureCursorVisible();
    m_window->updateCursor();
    m_dirty_display = true;
}

void Editor::handleNormalMode(const KeyPress& key) {
    if (key.isSpecialKey()) {
        switch (key.key) {
            case ESCAPE:
                clearCommandSequence();
                setMode(NORMAL);
                break;
            case ARROW_LEFT: moveLeft(); m_dirty_display = true; break;
            case ARROW_RIGHT: moveRight(); m_dirty_display = true; break;
            case ARROW_UP: moveUp(); m_dirty_display = true; break;
            case ARROW_DOWN: moveDown(); m_dirty_display = true; break;
            default: break;
        }
    } else if (key.isCharacter()) {
        std::string ch = key.utf8_char;
        
        // Handle digits for repeat count
        if (isDigit(ch) && (m_repeat_count > 0 || ch != "0")) {
            m_repeat_count = m_repeat_count * 10 + (ch[0] - '0');
            return;
        }
        
        // Handle command sequences
        if (!m_command_sequence.empty() || isValidCommandStart(ch)) {
            handleCommandSequence(ch);
            return;
        }
        
        // Simple single-character commands
        if (ch == "h") { moveLeft(); m_dirty_display = true; }
        else if (ch == "j") { moveDown(); m_dirty_display = true; }
        else if (ch == "k") { moveUp(); m_dirty_display = true; }
        else if (ch == "l") { moveRight(); m_dirty_display = true; }
        else if (ch == "w") { moveWordForward(); m_dirty_display = true; }
        else if (ch == "b") { moveWordBackward(); m_dirty_display = true; }
        else if (ch == "0") { moveLineBegin(); m_dirty_display = true; }
        else if (ch == "$") { moveLineEnd(); m_dirty_display = true; }
        else if (ch == "G") { moveLastLine(); m_dirty_display = true; }
        else if (ch == "i") enterInsertMode();
        else if (ch == "a") enterInsertModeAfter();
        else if (ch == "o") enterInsertModeNewLine();
        else if (ch == "O") enterInsertModeNewLineAbove();
        else if (ch == "x") deleteCharacter();
        else if (ch == "u") undoChange();
        else if (ch == ":") enterCommandMode();
        else if (ch == "/") searchForward();
        else if (ch == "?") searchBackward();
        else if (ch == "n") searchNext();
        else if (ch == "N") searchPrevious();
        else if (ch == "v") enterVisualMode();
        else if (ch == "V") enterVisualLineMode();
        
        // Clear repeat count after command execution
        m_repeat_count = 0;
    }
}

void Editor::handleInsertMode(const KeyPress& key) {
    if (key.isSpecialKey()) {
        switch (key.key) {
            case ESCAPE:
                setMode(NORMAL);
                break;
            case BACKSPACE:
                m_buffer->deleteCharBefore();
                m_dirty_display = true;
                break;
            case DELETE:
                m_buffer->deleteChar();
                m_dirty_display = true;
                break;
            case ENTER:
                m_buffer->splitLine();
                m_dirty_display = true;
                break;
            case TAB:
                // Insert tab as 4 spaces (configurable in future)
                m_buffer->insertString("    ");
                m_dirty_display = true;
                break;
            case ARROW_LEFT: moveLeft(); m_dirty_display = true; break;
            case ARROW_RIGHT: moveRight(); m_dirty_display = true; break;
            case ARROW_UP: moveUp(); m_dirty_display = true; break;
            case ARROW_DOWN: moveDown(); m_dirty_display = true; break;
            default: break;
        }
    } else if (key.isCharacter()) {
        m_buffer->insertString(key.utf8_char);
        m_dirty_display = true;
    }
}

void Editor::handleVisualMode(const KeyPress& key) {
    // Simplified visual mode - just movement and escape
    if (key.isSpecialKey() && key.key == ESCAPE) {
        setMode(NORMAL);
    } else {
        // Use normal mode movement for now
        handleNormalMode(key);
    }
}

void Editor::handleCommandMode(const KeyPress& key) {
    if (key.isSpecialKey()) {
        switch (key.key) {
            case ESCAPE:
                setMode(NORMAL);
                m_command_line.clear();
                break;
            case ENTER:
                executeCommand(m_command_line);
                setMode(NORMAL);
                m_command_line.clear();
                break;
            case BACKSPACE:
                if (!m_command_line.empty()) {
                    // Remove the last UTF-8 character properly
                    size_t char_count = utf8::length(m_command_line);
                    if (char_count > 0) {
                        m_command_line = utf8::substr(m_command_line, 0, char_count - 1);
                    }
                }
                break;
            default: break;
        }
    } else if (key.isCharacter()) {
        m_command_line += key.utf8_char;
    }
}

void Editor::handleSearchMode(const KeyPress& key) {
    // Similar to command mode for now
    handleCommandMode(key);
}

// Movement implementations
void Editor::moveLeft() { m_buffer->moveCursor(0, -1); }
void Editor::moveRight() { m_buffer->moveCursor(0, 1); }
void Editor::moveUp() { m_buffer->moveCursor(-1, 0); }
void Editor::moveDown() { m_buffer->moveCursor(1, 0); }

void Editor::moveWordForward() {
    BufferPosition next = m_buffer->getNextWord();
    m_buffer->setCursor(next);
}

void Editor::moveWordBackward() {
    BufferPosition prev = m_buffer->getPreviousWord();
    m_buffer->setCursor(prev);
}

void Editor::moveLineBegin() {
    BufferPosition pos = m_buffer->getLineBegin();
    m_buffer->setCursor(pos);
}

void Editor::moveLineEnd() {
    BufferPosition pos = m_buffer->getLineEnd();
    m_buffer->setCursor(pos);
}

void Editor::moveFirstLine() {
    BufferPosition pos = m_buffer->getBufferBegin();
    m_buffer->setCursor(pos);
}

void Editor::moveLastLine() {
    BufferPosition pos = m_buffer->getBufferEnd();
    m_buffer->setCursor(pos);
}

// Edit mode implementations
void Editor::enterInsertMode() { setMode(INSERT); }
void Editor::enterInsertModeAfter() { moveRight(); setMode(INSERT); }

void Editor::enterInsertModeNewLine() {
    m_buffer->insertLineAfter();
    setMode(INSERT);
}

void Editor::enterInsertModeNewLineAbove() {
    m_buffer->insertLine();
    setMode(INSERT);
}

void Editor::deleteCharacter() { m_buffer->deleteChar(); }
void Editor::deleteLine() { m_buffer->deleteLine(); }

void Editor::yankLine() {
    m_yank_buffer = m_buffer->yankLine();
    m_yank_line_mode = true;
    setStatusMessage("Yanked line");
}

void Editor::pasteAfter() {
    if (!m_yank_buffer.empty()) {
        m_buffer->pasteAfter(m_yank_buffer);
    }
}

void Editor::pasteBefore() {
    if (!m_yank_buffer.empty()) {
        m_buffer->pasteBefore(m_yank_buffer);
    }
}

void Editor::undoChange() { m_buffer->undo(); }

void Editor::enterCommandMode() {
    setMode(COMMAND);
    m_command_line.clear();
}

void Editor::searchForward() {
    setMode(SEARCH);
    m_search_forward = true;
    m_command_line.clear();
}

void Editor::searchBackward() {
    setMode(SEARCH);
    m_search_forward = false;
    m_command_line.clear();
}

void Editor::searchNext() {
    // TODO: Implement search
    setStatusMessage("Search next - not implemented");
}

void Editor::searchPrevious() {
    // TODO: Implement search  
    setStatusMessage("Search previous - not implemented");
}

void Editor::enterVisualMode() { setMode(VISUAL); }
void Editor::enterVisualLineMode() { setMode(VISUAL_LINE); }

void Editor::executeCommand(const std::string& command) {
    if (command.empty()) return;
    
    if (command == "q" || command == "quit") {
        if (m_buffer->isModified()) {
            setErrorMessage("No write since last change (use :q! to override)");
        } else {
            quit();
        }
    } else if (command == "q!" || command == "quit!") {
        quit();
    } else if (command == "w" || command == "write") {
        saveFile();
    } else if (command == "wq" || command == "x") {
        if (saveFile()) {
            quit();
        }
    } else if (command.substr(0, 2) == "w ") {
        saveFile(command.substr(2));
    } else if (command == "e" || command == "edit") {
        setErrorMessage("No filename specified");
    } else if (command == "e!" || command == "edit!") {
        // Reload current file, discarding changes
        if (m_buffer->getFilename().empty()) {
            setErrorMessage("No filename to reload");
        } else {
            openFile(m_buffer->getFilename());
        }
    } else if (command.substr(0, 2) == "e ") {
        std::string filename = command.substr(2);
        // Trim whitespace
        size_t start = filename.find_first_not_of(" \t");
        if (start != std::string::npos) {
            filename = filename.substr(start);
            size_t end = filename.find_last_not_of(" \t");
            if (end != std::string::npos) {
                filename = filename.substr(0, end + 1);
            }
        }
        
        if (filename.empty()) {
            setErrorMessage("No filename specified");
        } else {
            // Check if current buffer has unsaved changes
            if (m_buffer->isModified() && !m_buffer->getFilename().empty()) {
                setErrorMessage("No write since last change (use :e! to override)");
            } else {
                openFile(filename);
            }
        }
    } else if (command.substr(0, 3) == "e! ") {
        std::string filename = command.substr(3);
        // Trim whitespace
        size_t start = filename.find_first_not_of(" \t");
        if (start != std::string::npos) {
            filename = filename.substr(start);
            size_t end = filename.find_last_not_of(" \t");
            if (end != std::string::npos) {
                filename = filename.substr(0, end + 1);
            }
        }
        
        if (filename.empty()) {
            setErrorMessage("No filename specified");
        } else {
            openFile(filename);  // Force open, discarding changes
        }
    } else if (command == "ls" || command == "buffers") {
        listBuffers();
    } else if (command == "bn" || command == "bnext") {
        nextBuffer();
    } else if (command == "bp" || command == "bprev") {
        previousBuffer();
    } else if (command == "bd" || command == "bdelete") {
        closeBuffer();
    } else if (command == "bd!" || command == "bdelete!") {
        // Force close buffer - TODO: implement force close
        closeBuffer();
    } else if (command.substr(0, 2) == "b ") {
        std::string buffer_num_str = command.substr(2);
        try {
            int buffer_num = compat::stoi(buffer_num_str);
            switchToBuffer(buffer_num - 1);  // Convert to 0-based index
        } catch (const std::exception&) {
            setErrorMessage("Invalid buffer number: " + buffer_num_str);
        }
    } else {
        setErrorMessage("Unknown command: " + command);
    }
}

void Editor::initializeKeyBindings() {
    // Key bindings will be expanded later
}

void Editor::clearMessages() {
    if (!m_status_message.empty() || !m_error_message.empty()) {
        m_status_message.clear();
        m_error_message.clear();
        m_dirty_display = true;
    }
}

void Editor::movePage(bool down) {
    TerminalSize size = m_window->getSize();
    int lines = size.rows - 2; // Leave some overlap
    m_buffer->moveCursor(down ? lines : -lines, 0);
}

void Editor::handleCommandSequence(const std::string& key) {
    m_command_sequence += key;
    
    // Check for complete commands
    if (m_command_sequence == "gg") {
        for (int i = 0; i < (m_repeat_count > 0 ? m_repeat_count : 1); ++i) {
            moveFirstLine();
        }
        m_repeat_count = 0;
        clearCommandSequence();
    } else if (m_command_sequence == "dd") {
        for (int i = 0; i < (m_repeat_count > 0 ? m_repeat_count : 1); ++i) {
            deleteLine();
        }
        m_repeat_count = 0;
        clearCommandSequence();
    } else if (m_command_sequence == "yy") {
        for (int i = 0; i < (m_repeat_count > 0 ? m_repeat_count : 1); ++i) {
            yankLine();
        }
        m_repeat_count = 0;
        clearCommandSequence();
    } else if (m_command_sequence.length() == 1) {
        // Single character that might be part of a sequence
        if (key == "g" || key == "d" || key == "y") {
            // Wait for next character
            return;
        } else if (key == "p") {
            for (int i = 0; i < (m_repeat_count > 0 ? m_repeat_count : 1); ++i) {
                pasteAfter();
            }
            m_repeat_count = 0;
            clearCommandSequence();
        } else if (key == "P") {
            for (int i = 0; i < (m_repeat_count > 0 ? m_repeat_count : 1); ++i) {
                pasteBefore();
            }
            m_repeat_count = 0;
            clearCommandSequence();
        } else {
            // Invalid sequence
            clearCommandSequence();
        }
    } else {
        // Invalid or incomplete sequence
        clearCommandSequence();
    }
}

void Editor::executeCommandSequence() {
    // This method is called when a complete command sequence is ready
    clearCommandSequence();
}

void Editor::clearCommandSequence() {
    m_command_sequence.clear();
    m_pending_command.clear();
    m_repeat_count = 0;
}

bool Editor::isDigit(const std::string& key) const {
    return !key.empty() && key[0] >= '0' && key[0] <= '9';
}

bool Editor::isValidCommandStart(const std::string& key) const {
    return key == "g" || key == "d" || key == "y" || key == "p" || key == "P";
}

// Buffer management methods
void Editor::switchToBuffer(int buffer_index) {
    if (buffer_index >= 0 && buffer_index < static_cast<int>(m_buffers.size())) {
        m_current_buffer_index = buffer_index;
        m_buffer = m_buffers[buffer_index];
        m_window->setBuffer(m_buffer);
        
        // Set up syntax highlighting for this buffer
        if (m_syntax_manager && !m_buffer->getFilename().empty()) {
            ISyntaxHighlighter* highlighter = m_syntax_manager->getHighlighterForFile(m_buffer->getFilename());
            m_window->setSyntaxHighlighter(highlighter);
        }
        
        std::string filename = m_buffer->getFilename();
        if (filename.empty()) {
            filename = "[No Name]";
        }
        setStatusMessage("Switched to buffer " + compat::to_string(buffer_index + 1) + ": " + filename);
        m_dirty_display = true;
    } else {
        setErrorMessage("No buffer " + compat::to_string(buffer_index + 1));
    }
}

void Editor::nextBuffer() {
    if (m_buffers.size() > 1) {
        int next_index = (m_current_buffer_index + 1) % m_buffers.size();
        switchToBuffer(next_index);
    }
}

void Editor::previousBuffer() {
    if (m_buffers.size() > 1) {
        int prev_index = (m_current_buffer_index - 1 + m_buffers.size()) % m_buffers.size();
        switchToBuffer(prev_index);
    }
}

bool Editor::closeBuffer(int buffer_index) {
    if (buffer_index == -1) {
        buffer_index = m_current_buffer_index;
    }
    
    if (buffer_index < 0 || buffer_index >= static_cast<int>(m_buffers.size())) {
        setErrorMessage("No buffer " + compat::to_string(buffer_index + 1));
        return false;
    }
    
    // Check if buffer has unsaved changes
    if (m_buffers[buffer_index]->isModified()) {
        setErrorMessage("No write since last change (use :bd! to override)");
        return false;
    }
    
    // Don't close the last buffer
    if (m_buffers.size() == 1) {
        setErrorMessage("Cannot close last buffer");
        return false;
    }
    
    // Remove the buffer
    m_buffers.erase(m_buffers.begin() + buffer_index);
    
    // Adjust current buffer index
    if (m_current_buffer_index >= buffer_index && m_current_buffer_index > 0) {
        m_current_buffer_index--;
    }
    if (m_current_buffer_index >= static_cast<int>(m_buffers.size())) {
        m_current_buffer_index = m_buffers.size() - 1;
    }
    
    // Switch to the new current buffer
    m_buffer = m_buffers[m_current_buffer_index];
    m_window->setBuffer(m_buffer);
    
    setStatusMessage("Buffer closed. Now showing buffer " + compat::to_string(m_current_buffer_index + 1));
    m_dirty_display = true;
    return true;
}

void Editor::listBuffers() {
    std::string buffer_list = "Buffers:\n";
    for (size_t i = 0; i < m_buffers.size(); ++i) {
        std::string marker = (i == static_cast<size_t>(m_current_buffer_index)) ? "%" : " ";
        std::string modified = m_buffers[i]->isModified() ? "+" : " ";
        std::string filename = m_buffers[i]->getFilename();
        if (filename.empty()) {
            filename = "[No Name]";
        }
        
        buffer_list += "  " + compat::to_string(i + 1) + marker + modified + " " + filename + "\n";
    }
    
    setStatusMessage(buffer_list);
}

} // namespace subzero