#pragma once
#include "buffer.h"
#include "terminal.h"
#include "terminal_types.h"
#include "syntax_highlighter.h"
#include <memory>

namespace subzero {

class Window {
private:
    std::shared_ptr<Buffer> m_buffer;
    std::shared_ptr<ITerminal> m_terminal;
    
    // Window dimensions and position
    Position m_window_pos;      // Terminal position of window
    TerminalSize m_window_size; // Size of window
    
    // Viewport (what part of buffer is visible)
    size_t m_top_line;          // First visible line in buffer
    size_t m_left_column;       // First visible column in buffer
    
    // Cursor display
    Position m_screen_cursor;   // Cursor position on screen
    
    // Display settings
    bool m_show_line_numbers;
    bool m_wrap_lines;
    size_t m_tab_width;
    
    // Syntax highlighting
    ISyntaxHighlighter* m_syntax_highlighter;
    
public:
    Window(std::shared_ptr<ITerminal> terminal, std::shared_ptr<Buffer> buffer);
    
    // Window management
    void setPosition(const Position& pos) { m_window_pos = pos; }
    void setSize(const TerminalSize& size) { m_window_size = size; }
    const Position& getPosition() const { return m_window_pos; }
    const TerminalSize& getSize() const { return m_window_size; }
    
    // Buffer management
    void setBuffer(std::shared_ptr<Buffer> buffer);
    std::shared_ptr<Buffer> getBuffer() const { return m_buffer; }
    
    // Display control
    void setShowLineNumbers(bool show) { m_show_line_numbers = show; }
    void setWrapLines(bool wrap) { m_wrap_lines = wrap; }
    void setTabWidth(size_t width) { m_tab_width = width; }
    
    // Syntax highlighting
    void setSyntaxHighlighter(ISyntaxHighlighter* highlighter) { m_syntax_highlighter = highlighter; }
    
    // Viewport operations
    void scrollUp(size_t lines = 1);
    void scrollDown(size_t lines = 1);
    void scrollLeft(size_t columns = 1);
    void scrollRight(size_t columns = 1);
    void scrollToLine(size_t line);
    void centerOnCursor();
    
    // Rendering
    void render();
    void renderLine(size_t buffer_line, size_t screen_row);
    void updateCursor();
    
    // Coordinate conversion
    Position bufferToScreen(const BufferPosition& buffer_pos) const;
    BufferPosition screenToBuffer(const Position& screen_pos) const;
    
    // Cursor management
    void ensureCursorVisible();
    Position getScreenCursor() const { return m_screen_cursor; }
    
private:
    void calculateScreenCursor();
    size_t getTextAreaWidth() const;
    size_t getLineNumberWidth() const;
    std::string formatLineNumber(size_t line_num) const;
    std::string expandTabs(const std::string& line) const;
    void renderLineNumbers(size_t screen_row, size_t buffer_line);
    void renderText(const std::string& text, size_t screen_row, size_t start_col);
    void renderSyntaxHighlightedText(const std::string& text, size_t buffer_line, size_t screen_row, size_t start_col);
};

} // namespace subzero