#pragma once
#include "buffer.h"
#include "window.h"
#include "terminal.h"
#include "syntax_highlighter_manager.h"
#include <memory>
#include <string>
#include <functional>

namespace subzero {

enum class EditorMode {
    NORMAL,     // Normal vi mode - movement and commands
    INSERT,     // Insert mode - typing text
    VISUAL,     // Visual selection mode
    VISUAL_LINE,// Visual line selection mode
    COMMAND,    // Command line mode (:w, :q, etc.)
    SEARCH      // Search mode (/, ?)
};

struct KeyBinding {
    std::string keys;
    std::function<void()> action;
    EditorMode mode;
    std::string description;
};

class Editor {
private:
    std::shared_ptr<ITerminal> m_terminal;
    std::shared_ptr<Buffer> m_buffer;  // Current active buffer
    std::shared_ptr<Window> m_window;
    
    // Buffer management
    std::vector<std::shared_ptr<Buffer>> m_buffers;
    int m_current_buffer_index;
    
    EditorMode m_mode;
    EditorMode m_previous_mode;
    
    // Command state
    std::string m_command_line;
    std::string m_search_pattern;
    std::string m_last_search;
    bool m_search_forward;
    
    // Key bindings
    std::vector<KeyBinding> m_key_bindings;
    
    // Status and messages
    std::string m_status_message;
    std::string m_error_message;
    
    // Editor state
    bool m_running;
    bool m_dirty_display;
    
    // Command sequence handling
    std::string m_command_sequence;
    std::string m_pending_command;
    
    // Yank buffer (clipboard)
    std::string m_yank_buffer;
    bool m_yank_line_mode;
    
    // Repeat and count
    int m_repeat_count;
    std::unique_ptr<SyntaxHighlighterManager> m_syntax_manager;
    
    // Last command for repeat
    std::string m_last_command;
    
public:
    Editor(std::shared_ptr<ITerminal> terminal);
    ~Editor() = default;
    
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
    void applyRepeatCount(std::function<void()> action);
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