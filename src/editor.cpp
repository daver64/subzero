#include "editor.h"
#include <iostream>
#include <sstream>
#include <cctype>  // For tolower, isalnum

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
    , m_fast_mode(false)
    , m_render_delay_counter(0)
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
    
    // Ensure minimum terminal size and leave space for status bar
    int window_rows = (terminal_size.rows > 1) ? terminal_size.rows - 1 : 24;
    int window_cols = (terminal_size.cols > 0) ? terminal_size.cols : 80;
    
    m_window->setPosition(Position(0, 0));
    m_window->setSize(TerminalSize(window_rows, window_cols));
    
    while (m_running) {
        // Check if we should defer rendering during rapid typing
        if (m_dirty_display) {
            if (m_render_delay_counter > 0) {
                m_render_delay_counter--;
                // Skip rendering this frame to improve typing performance
            } else {
                render();
                m_dirty_display = false;
                m_fast_mode = false;  // Exit fast mode after rendering
            }
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
    
    // Ensure syntax highlighter is set correctly (skip in fast mode for performance)
    if (!m_fast_mode && m_syntax_manager && m_buffer && !m_buffer->getFilename().empty()) {
        ISyntaxHighlighter* highlighter = m_syntax_manager->getHighlighterForFile(m_buffer->getFilename());
        m_window->setSyntaxHighlighter(highlighter);
    } else if (m_fast_mode) {
        // Disable syntax highlighting in fast mode
        m_window->setSyntaxHighlighter(NULL);
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
    
    // Ensure valid terminal size
    if (terminal_size.rows <= 0 || terminal_size.cols <= 0) {
        return;  // Can't render if size is invalid
    }
    
    // Status line position (bottom row)
    Position status_pos(terminal_size.rows - 1, 0);
    
    // Clear status line with high-contrast colors for visibility
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
        else if (ch == "*") searchWordForward();
        else if (ch == "#") searchWordBackward();
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
                m_dirty_display = true;  // Full redraw for backspace (line may change)
                break;
            case DELETE:
                m_buffer->deleteChar();
                m_dirty_display = true;  // Full redraw for delete
                break;
            case ENTER:
                m_buffer->splitLine();
                m_dirty_display = true;  // Full redraw for new line
                break;
            case TAB:
                // Insert tab as 4 spaces (configurable in future)
                m_buffer->insertString("    ");
                m_dirty_display = true;  // Full redraw for tab
                break;
            case ARROW_LEFT: moveLeft(); break;  // No redraw needed for movement
            case ARROW_RIGHT: moveRight(); break;
            case ARROW_UP: moveUp(); break;
            case ARROW_DOWN: moveDown(); break;
            default: break;
        }
    } else if (key.isCharacter()) {
        m_buffer->insertString(key.utf8_char);
        
        // Enable fast mode and defer rendering for better typing performance
        m_fast_mode = true;
        m_render_delay_counter = 2;  // Defer for 2 input cycles
        m_dirty_display = true;
        
        // Immediate cursor update for responsive feel (minimal cost)
        if (m_window) {
            m_window->updateCursor();
            // Skip terminal refresh - will be done when rendering resumes
        }
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
    if (key.isSpecialKey() && key.key == ENTER) {
        // Execute search
        if (!m_command_line.empty()) {
            m_search_pattern = m_command_line;
            m_last_search = m_search_pattern;
            executeSearch();
        }
        setMode(NORMAL);
        m_command_line.clear();
    } else if (key.isSpecialKey() && key.key == ESCAPE) {
        // Cancel search
        setMode(NORMAL);
        m_command_line.clear();
    } else if (key.isSpecialKey() && key.key == BACKSPACE) {
        // Delete character in search pattern
        if (!m_command_line.empty()) {
            m_command_line.erase(m_command_line.length() - 1);
        }
    } else if (key.isCharacter()) {
        // Add character to search pattern
        m_command_line += key.utf8_char;
    }
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

void Editor::deleteCharacter() { 
    m_buffer->deleteChar(); 
    m_dirty_display = true;  // Fix: Update screen after character deletion
}
void Editor::deleteLine() { 
    m_buffer->deleteLine(); 
    m_dirty_display = true;  // Fix: Update screen after line deletion
}

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
    if (m_last_search.empty()) {
        setStatusMessage("No previous search pattern");
        return;
    }
    
    m_search_pattern = m_last_search;
    if (findInBuffer(m_search_pattern, m_search_forward, true)) {
        setStatusMessage("Found: " + m_search_pattern);
    } else {
        setStatusMessage("Pattern not found: " + m_search_pattern);
    }
}

void Editor::searchPrevious() {
    if (m_last_search.empty()) {
        setStatusMessage("No previous search pattern");
        return;
    }
    
    m_search_pattern = m_last_search;
    if (findInBuffer(m_search_pattern, !m_search_forward, true)) {
        setStatusMessage("Found: " + m_search_pattern);
    } else {
        setStatusMessage("Pattern not found: " + m_search_pattern);
    }
}

void Editor::searchWordForward() {
    std::string word = getCurrentWord();
    if (word.empty()) {
        setStatusMessage("No word under cursor");
        return;
    }
    
    m_search_pattern = word;
    m_last_search = word;
    m_search_forward = true;
    
    if (findInBuffer(m_search_pattern, true, true)) {
        setStatusMessage("Found: " + m_search_pattern);
    } else {
        setStatusMessage("Pattern not found: " + m_search_pattern);
    }
}

void Editor::searchWordBackward() {
    std::string word = getCurrentWord();
    if (word.empty()) {
        setStatusMessage("No word under cursor");
        return;
    }
    
    m_search_pattern = word;
    m_last_search = word;
    m_search_forward = false;
    
    if (findInBuffer(m_search_pattern, false, true)) {
        setStatusMessage("Found: " + m_search_pattern);
    } else {
        setStatusMessage("Pattern not found: " + m_search_pattern);
    }
}

// Search implementation methods
void Editor::executeSearch() {
    if (m_search_pattern.empty()) {
        setStatusMessage("Empty search pattern");
        return;
    }
    
    if (findInBuffer(m_search_pattern, m_search_forward, true)) {
        setStatusMessage("Found: " + m_search_pattern);
    } else {
        setStatusMessage("Pattern not found: " + m_search_pattern);
    }
}

bool Editor::findInBuffer(const std::string& pattern, bool forward, bool wrap_around) {
    if (!m_buffer || pattern.empty()) {
        return false;
    }
    
    BufferPosition current = m_buffer->getCursor();
    int start_line = current.line;
    int start_col = current.column;
    
    // For forward search, start from next position
    // For backward search, start from current position
    if (forward) {
        start_col++;
    }
    
    // Search from current position
    for (int line = start_line; line < (int)m_buffer->getLineCount(); line++) {
        std::string line_text = m_buffer->getLine(line);
        int search_start = (line == start_line) ? start_col : 0;
        
        if (forward) {
            int found_pos;
            if (findInLine(line_text, pattern, search_start, found_pos, true)) {
                m_buffer->setCursor(BufferPosition(line, found_pos));
                return true;
            }
        } else {
            // Backward search in current line
            if (line == start_line) {
                for (int col = start_col - 1; col >= 0; col--) {
                    if (matchesAtPosition(line_text, pattern, col, true)) {
                        m_buffer->setCursor(BufferPosition(line, col));
                        return true;
                    }
                }
            } else {
                // Search entire line from end
                for (int col = (int)line_text.length() - (int)pattern.length(); col >= 0; col--) {
                    if (matchesAtPosition(line_text, pattern, col, true)) {
                        m_buffer->setCursor(BufferPosition(line, col));
                        return true;
                    }
                }
            }
        }
    }
    
    // If wrap_around is enabled and we haven't found anything, search from beginning/end
    if (wrap_around) {
        if (forward) {
            // Search from beginning to current position
            for (int line = 0; line <= start_line; line++) {
                std::string line_text = m_buffer->getLine(line);
                int search_end = (line == start_line) ? start_col : (int)line_text.length();
                
                int found_pos;
                if (findInLine(line_text, pattern, 0, found_pos, true) && found_pos < search_end) {
                    m_buffer->setCursor(BufferPosition(line, found_pos));
                    return true;
                }
            }
        } else {
            // Search from end to current position
            for (int line = m_buffer->getLineCount() - 1; line >= start_line; line--) {
                std::string line_text = m_buffer->getLine(line);
                
                if (line == start_line) {
                    // Already searched this part
                    continue;
                } else {
                    // Search entire line from end
                    for (int col = (int)line_text.length() - (int)pattern.length(); col >= 0; col--) {
                        if (matchesAtPosition(line_text, pattern, col, true)) {
                            m_buffer->setCursor(BufferPosition(line, col));
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

bool Editor::findInLine(const std::string& line, const std::string& pattern, int start_pos, int& found_pos, bool case_sensitive) {
    if (start_pos < 0 || start_pos >= (int)line.length() || pattern.empty()) {
        return false;
    }
    
    for (int pos = start_pos; pos <= (int)line.length() - (int)pattern.length(); pos++) {
        if (matchesAtPosition(line, pattern, pos, case_sensitive)) {
            found_pos = pos;
            return true;
        }
    }
    
    return false;
}

bool Editor::matchesAtPosition(const std::string& text, const std::string& pattern, size_t pos, bool case_sensitive) {
    if (pos + pattern.length() > text.length()) {
        return false;
    }
    
    for (size_t i = 0; i < pattern.length(); i++) {
        char text_char = text[pos + i];
        char pattern_char = pattern[i];
        
        if (!case_sensitive) {
            text_char = tolower(text_char);
            pattern_char = tolower(pattern_char);
        }
        
        if (text_char != pattern_char) {
            return false;
        }
    }
    
    return true;
}

std::string Editor::getCurrentWord() {
    if (!m_buffer) {
        return "";
    }
    
    BufferPosition cursor = m_buffer->getCursor();
    std::string line = m_buffer->getLine(cursor.line);
    
    if ((int)cursor.column >= (int)line.length()) {
        return "";
    }
    
    // Find word boundaries
    int start = cursor.column;
    int end = cursor.column;
    
    // Move start back to beginning of word
    while (start > 0 && (isalnum(line[start - 1]) || line[start - 1] == '_')) {
        start--;
    }
    
    // Move end forward to end of word
    while (end < (int)line.length() && (isalnum(line[end]) || line[end] == '_')) {
        end++;
    }
    
    if (start == end) {
        return "";
    }
    
    return line.substr(start, end - start);
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
    } else if (command == "help" || command == "h") {
        showHelp();
    } else {
        setErrorMessage("Unknown command: " + command);
    }
}

void Editor::showHelp() {
    // Create a comprehensive help message
    std::string help_text = "SubZero Editor - Command Reference\n\n";
    
    help_text += "=== COMMAND MODE (type : to enter) ===\n";
    help_text += "File Operations:\n";
    help_text += "  :w                 - Save file\n";
    help_text += "  :w filename        - Save as filename\n";
    help_text += "  :q                 - Quit (warns if unsaved changes)\n";
    help_text += "  :q!                - Force quit (discard changes)\n";
    help_text += "  :wq, :x            - Save and quit\n";
    help_text += "  :e filename        - Edit file\n";
    help_text += "  :e!                - Reload current file (discard changes)\n";
    help_text += "  :e! filename       - Force edit file (discard changes)\n\n";
    
    help_text += "Buffer Management:\n";
    help_text += "  :ls, :buffers      - List all open buffers\n";
    help_text += "  :b N               - Switch to buffer N (1-based)\n";
    help_text += "  :bn, :bnext        - Next buffer\n";
    help_text += "  :bp, :bprev        - Previous buffer\n";
    help_text += "  :bd, :bdelete      - Close current buffer\n";
    help_text += "  :bd!               - Force close buffer\n\n";
    
    help_text += "Help:\n";
    help_text += "  :help, :h          - Show this help\n\n";
    
    help_text += "=== NORMAL MODE (default mode) ===\n";
    help_text += "Movement:\n";
    help_text += "  h, j, k, l         - Left, Down, Up, Right\n";
    help_text += "  Arrow keys         - Move cursor\n";
    help_text += "  w                  - Next word\n";
    help_text += "  b                  - Previous word\n";
    help_text += "  0                  - Beginning of line\n";
    help_text += "  $                  - End of line\n";
    help_text += "  gg                 - Go to first line\n";
    help_text += "  G                  - Go to last line\n\n";
    
    help_text += "Editing:\n";
    help_text += "  i                  - Insert mode at cursor\n";
    help_text += "  I                  - Insert at beginning of line\n";
    help_text += "  a                  - Insert after cursor\n";
    help_text += "  A                  - Insert at end of line\n";
    help_text += "  o                  - Open new line below\n";
    help_text += "  O                  - Open new line above\n";
    help_text += "  x                  - Delete character\n";
    help_text += "  dd                 - Delete line\n";
    help_text += "  yy                 - Copy line\n";
    help_text += "  p                  - Paste\n";
    help_text += "  u                  - Undo\n\n";
    
    help_text += "Search:\n";
    help_text += "  /pattern           - Search forward\n";
    help_text += "  ?pattern           - Search backward\n";
    help_text += "  n                  - Next search result\n";
    help_text += "  N                  - Previous search result\n";
    help_text += "  *                  - Search word under cursor (forward)\n";
    help_text += "  #                  - Search word under cursor (backward)\n\n";
    
    help_text += "Visual Mode:\n";
    help_text += "  v                  - Character visual mode\n";
    help_text += "  V                  - Line visual mode\n\n";
    
    help_text += "=== INSERT MODE ===\n";
    help_text += "  ESC                - Return to normal mode\n";
    help_text += "  Printable chars    - Insert text\n";
    help_text += "  Backspace          - Delete previous character\n";
    help_text += "  Delete             - Delete character at cursor\n";
    help_text += "  Enter              - New line\n";
    help_text += "  Tab                - Insert 4 spaces\n";
    help_text += "  Arrow keys         - Move cursor\n\n";
    
    help_text += "=== SEARCH MODE ===\n";
    help_text += "  Enter              - Execute search\n";
    help_text += "  ESC                - Cancel search\n";
    help_text += "  Backspace          - Delete character\n";
    help_text += "  Printable chars    - Add to search pattern\n\n";
    
    help_text += "=== FEATURES ===\n";
    help_text += "- Syntax highlighting for C/C++ and Markdown files\n";
    help_text += "- UTF-8 text support\n";
    help_text += "- Multi-buffer editing\n";
    help_text += "- Cross-platform (Linux, Windows, Atari MiNTOS)\n";
    help_text += "- Performance optimized for retro systems\n";
    
    // Create a temporary buffer with help content
    shared_ptr<Buffer> help_buffer(new Buffer());
    help_buffer->setFilename("*help*");
    
    // Write help text to a temporary string stream and load it like a file
    std::istringstream help_stream(help_text);
    help_buffer->loadFromStream(help_stream);
    
    // Add to buffer list and switch to it
    m_buffers.push_back(help_buffer);
    switchToBuffer(m_buffers.size() - 1);
    
    setStatusMessage("Help buffer opened - :q to close");
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
        
        // Force immediate screen clear and redraw when switching buffers
        render();
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