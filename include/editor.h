#pragma once
#include "buffer.h"
#include "window.h"
#include "terminal.h"
#include "syntax_highlighter_manager.h"
#include "compat.h"
#include <string>
#include <functional>

namespace subzero {

enum EditorMode {
    NORMAL,     // Normal vi mode - movement and commands
    INSERT,     // Insert mode - typing text
    VISUAL,     // Visual selection mode
    VISUAL_LINE,// Visual line selection mode
    COMMAND,    // Command line mode (:w, :q, etc.)
    SEARCH      // Search mode (/, ?)
};

// Remove KeyBinding struct for C++98 compatibility

class Editor {
private:
    shared_ptr<ITerminal> m_terminal;
    shared_ptr<Buffer> m_buffer;  // Current active buffer
    shared_ptr<Window> m_window;
    
    // Buffer management
    std::vector<shared_ptr<Buffer> > m_buffers;
    int m_current_buffer_index;
    
    EditorMode m_mode;
    EditorMode m_previous_mode;
    
    // Command state
    std::string m_command_line;
    std::string m_search_pattern;
    std::string m_last_search;
    bool m_search_forward;
    
    // Status and messages
    std::string m_status_message;
    std::string m_error_message;
    
    // Editor state
    bool m_running;
    bool m_dirty_display;
    bool m_fast_mode;  // Disable expensive operations during rapid typing
    
    // Command sequence handling
    std::string m_command_sequence;
    std::string m_pending_command;
    
    // Yank buffer (clipboard)
    std::string m_yank_buffer;
    bool m_yank_line_mode;
    
    // Repeat and count
    int m_repeat_count;
    SyntaxHighlighterManager* m_syntax_manager;
    
    // Last command for repeat
    std::string m_last_command;
    
public:
    Editor(shared_ptr<ITerminal> terminal);
    ~Editor();  // Need to delete raw pointer
    
    // Main editor loop
    void run();
    bool isRunning() const { return m_running; }
    void quit() { m_running = false; }
    
    // File operations
    bool openFile(const std::string& filename);
    bool saveFile(const std::string& filename = "");
    bool newFile();
    
    // Buffer management
    void switchToBuffer(int buffer_index);
    void nextBuffer();
    void previousBuffer();
    bool closeBuffer(int buffer_index = -1);  // -1 for current buffer
    void listBuffers();
    int getCurrentBufferIndex() const { return m_current_buffer_index; }
    size_t getBufferCount() const { return m_buffers.size(); }
    
    // Mode management
    EditorMode getMode() const { return m_mode; }
    void setMode(EditorMode mode);
    std::string getModeString() const;
    
    // Display
    void render();
    void renderStatusBar();
    void setStatusMessage(const std::string& message);
    void setErrorMessage(const std::string& message);
    
    // Input handling
    void handleInput();
    void handleNormalMode(const KeyPress& key);
    void handleInsertMode(const KeyPress& key);
    void handleVisualMode(const KeyPress& key);
    void handleCommandMode(const KeyPress& key);
    void handleSearchMode(const KeyPress& key);
    
    // Movement commands (Normal mode)
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();
    void moveWordForward();
    void moveWordBackward();
    void moveLineBegin();
    void moveLineEnd();
    void moveFirstLine();
    void moveLastLine();
    void movePage(bool down);
    
    // Edit commands (Normal mode)
    void enterInsertMode();
    void enterInsertModeAfter();
    void enterInsertModeNewLine();
    void enterInsertModeNewLineAbove();
    void deleteCharacter();
    void deleteWord();
    void deleteLine();
    void deleteToEndOfLine();
    void yankLine();
    void yankWord();
    void pasteBefore();
    void pasteAfter();
    void undoChange();
    void redoChange();
    
    // Search commands
    void searchForward();
    void searchBackward();
    void searchNext();
    void searchPrevious();
    void searchWordForward();
    void searchWordBackward();
    
    // Search helper methods
    bool findInBuffer(const std::string& pattern, bool forward, bool wrap_around = true);
    bool findInLine(const std::string& line, const std::string& pattern, int start_pos, int& found_pos, bool case_sensitive = true);
    bool matchesAtPosition(const std::string& text, const std::string& pattern, size_t pos, bool case_sensitive = true);
    std::string getCurrentWord();
    void executeSearch();
    
    // Command mode
    void enterCommandMode();
    void executeCommand(const std::string& command);
    
    // Visual mode
    void enterVisualMode();
    void enterVisualLineMode();
    
private:
    void initializeKeyBindings();
    void setupNormalModeBindings();
    void parseRepeatCount(const KeyPress& key);
    // Remove applyRepeatCount for C++98 compatibility
    void clearMessages();
    TerminalSize getEditorArea() const;
    void refreshDisplay();
    
    // Command sequence handling
    void handleCommandSequence(const std::string& key);
    void executeCommandSequence();
    void clearCommandSequence();
    bool isDigit(const std::string& key) const;
    bool isValidCommandStart(const std::string& key) const;
};

} // namespace subzero