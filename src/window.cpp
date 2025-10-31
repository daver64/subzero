#include "window.h"
#include "utf8_utils.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace subzero {

Window::Window(shared_ptr<ITerminal> terminal, shared_ptr<Buffer> buffer)
    : m_buffer(buffer)
    , m_terminal(terminal)
    , m_window_pos(0, 0)
    , m_window_size(0, 0)
    , m_top_line(0)
    , m_left_column(0)
    , m_screen_cursor(0, 0)
    , m_show_line_numbers(true)
    , m_wrap_lines(false)
    , m_tab_width(4)
    , m_syntax_highlighter(NULL)
{
    if (m_terminal) {
        m_window_size = m_terminal->getSize();
    }
}

void Window::setBuffer(shared_ptr<Buffer> buffer) {
    m_buffer = buffer;
    m_top_line = 0;
    m_left_column = 0;
    calculateScreenCursor();
}

void Window::scrollUp(size_t lines) {
    if (m_top_line >= lines) {
        m_top_line -= lines;
    } else {
        m_top_line = 0;
    }
    calculateScreenCursor();
}

void Window::scrollDown(size_t lines) {
    if (!m_buffer) return;
    
    size_t max_top_line = m_buffer->getLineCount();
    if (max_top_line > static_cast<size_t>(m_window_size.rows)) {
        max_top_line -= m_window_size.rows;
    } else {
        max_top_line = 0;
    }
    
    m_top_line = std::min(m_top_line + lines, max_top_line);
    calculateScreenCursor();
}

void Window::scrollLeft(size_t columns) {
    if (m_left_column >= columns) {
        m_left_column -= columns;
    } else {
        m_left_column = 0;
    }
    calculateScreenCursor();
}

void Window::scrollRight(size_t columns) {
    m_left_column += columns;
    calculateScreenCursor();
}

void Window::scrollToLine(size_t line) {
    if (!m_buffer) return;
    
    if (line < m_buffer->getLineCount()) {
        m_top_line = line;
        calculateScreenCursor();
    }
}

void Window::centerOnCursor() {
    if (!m_buffer) return;
    
    const BufferPosition& cursor = m_buffer->getCursor();
    
    // Center vertically
    if (cursor.line >= static_cast<size_t>(m_window_size.rows / 2)) {
        m_top_line = cursor.line - m_window_size.rows / 2;
    } else {
        m_top_line = 0;
    }
    
    // Center horizontally (if not wrapping)
    if (!m_wrap_lines) {
        size_t text_width = getTextAreaWidth();
        if (cursor.column >= text_width / 2) {
            m_left_column = cursor.column - text_width / 2;
        } else {
            m_left_column = 0;
        }
    }
    
    calculateScreenCursor();
}

void Window::render() {
    if (!m_terminal || !m_buffer) return;
    
    // Clear window area
    for (int row = 0; row < m_window_size.rows; ++row) {
        for (int col = 0; col < m_window_size.cols; ++col) {
            Position pos(m_window_pos.row + row, m_window_pos.col + col);
            m_terminal->putChar(" ", pos);
        }
    }
    
    // Render buffer lines
    for (size_t screen_row = 0; screen_row < static_cast<size_t>(m_window_size.rows); ++screen_row) {
        size_t buffer_line = m_top_line + screen_row;
        renderLine(buffer_line, screen_row);
    }
    
    updateCursor();
}

void Window::renderLine(size_t buffer_line, size_t screen_row) {
    if (!m_terminal || !m_buffer) return;
    
    Position line_pos(m_window_pos.row + screen_row, m_window_pos.col);
    
    // Render line numbers if enabled
    if (m_show_line_numbers) {
        renderLineNumbers(screen_row, buffer_line);
    }
    
    // Render line content
    if (buffer_line < m_buffer->getLineCount()) {
        std::string line = m_buffer->getLine(buffer_line);
        line = expandTabs(line);
        
        // Handle horizontal scrolling
        if (!m_wrap_lines && line.length() > m_left_column) {
            size_t start_col = getLineNumberWidth();
            std::string visible_text = utf8::substr(line, m_left_column, getTextAreaWidth());
            
            if (m_syntax_highlighter) {
                renderSyntaxHighlightedText(visible_text, buffer_line, screen_row, start_col);
            } else {
                renderText(visible_text, screen_row, start_col);
            }
        }
    }
}

void Window::updateCursor() {
    if (!m_terminal) return;
    
    calculateScreenCursor();
    m_terminal->setCursor(Position(
        m_window_pos.row + m_screen_cursor.row,
        m_window_pos.col + m_screen_cursor.col
    ));
}

Position Window::bufferToScreen(const BufferPosition& buffer_pos) const {
    int screen_row = static_cast<int>(buffer_pos.line) - static_cast<int>(m_top_line);
    int screen_col = static_cast<int>(buffer_pos.column) - static_cast<int>(m_left_column);
    
    if (m_show_line_numbers) {
        screen_col += static_cast<int>(getLineNumberWidth());
    }
    
    return Position(screen_row, screen_col);
}

BufferPosition Window::screenToBuffer(const Position& screen_pos) const {
    size_t buffer_line = m_top_line + screen_pos.row;
    size_t buffer_col = m_left_column + screen_pos.col;
    
    if (m_show_line_numbers && screen_pos.col >= static_cast<int>(getLineNumberWidth())) {
        buffer_col -= getLineNumberWidth();
    }
    
    return BufferPosition(buffer_line, buffer_col);
}

void Window::ensureCursorVisible() {
    if (!m_buffer) return;
    
    const BufferPosition& cursor = m_buffer->getCursor();
    
    // Ensure cursor is vertically visible
    if (cursor.line < m_top_line) {
        m_top_line = cursor.line;
    } else if (cursor.line >= m_top_line + static_cast<size_t>(m_window_size.rows)) {
        m_top_line = cursor.line - m_window_size.rows + 1;
    }
    
    // Ensure cursor is horizontally visible (if not wrapping)
    if (!m_wrap_lines) {
        size_t text_width = getTextAreaWidth();
        if (cursor.column < m_left_column) {
            m_left_column = cursor.column;
        } else if (cursor.column >= m_left_column + text_width) {
            m_left_column = cursor.column - text_width + 1;
        }
    }
    
    calculateScreenCursor();
}

void Window::calculateScreenCursor() {
    if (!m_buffer) {
        m_screen_cursor = Position(0, 0);
        return;
    }
    
    const BufferPosition& cursor = m_buffer->getCursor();
    Position screen_pos = bufferToScreen(cursor);
    
    // Clamp to window bounds
    screen_pos.row = std::max(0, std::min(screen_pos.row, m_window_size.rows - 1));
    screen_pos.col = std::max(0, std::min(screen_pos.col, m_window_size.cols - 1));
    
    m_screen_cursor = screen_pos;
}

size_t Window::getTextAreaWidth() const {
    size_t total_width = static_cast<size_t>(m_window_size.cols);
    size_t line_num_width = m_show_line_numbers ? getLineNumberWidth() : 0;
    
    if (total_width > line_num_width) {
        return total_width - line_num_width;
    }
    return 0;
}

size_t Window::getLineNumberWidth() const {
    if (!m_show_line_numbers || !m_buffer) return 0;
    
    size_t line_count = m_buffer->getLineCount();
    size_t digits = 1;
    while (line_count >= 10) {
        line_count /= 10;
        digits++;
    }
    
    return digits + 2; // Add space for padding
}

std::string Window::formatLineNumber(size_t line_num) const {
    std::ostringstream oss;
    oss << std::setw(static_cast<int>(getLineNumberWidth() - 1)) << std::right << (line_num + 1) << " ";
    return oss.str();
}

std::string Window::expandTabs(const std::string& line) const {
    std::string result;
    size_t column = 0;
    
    for (size_t i = 0; i < line.length(); ) {
        if (line[i] == '\t') {
            size_t spaces = m_tab_width - (column % m_tab_width);
            result += std::string(spaces, ' ');
            column += spaces;
            i++;
        } else {
            size_t char_bytes = utf8::charByteLength(line, i);
            if (char_bytes > 0) {
                result += line.substr(i, char_bytes);
                column++;
                i += char_bytes;
            } else {
                i++; // Skip invalid byte
            }
        }
    }
    
    return result;
}

void Window::renderLineNumbers(size_t screen_row, size_t buffer_line) {
    if (!m_terminal || !m_show_line_numbers) return;
    
    Position pos(m_window_pos.row + screen_row, m_window_pos.col);
    
    if (buffer_line < m_buffer->getLineCount()) {
        std::string line_num = formatLineNumber(buffer_line);
        m_terminal->putStringWithColor(line_num, pos, Color::CYAN, Color::BLACK);
    }
}

void Window::renderText(const std::string& text, size_t screen_row, size_t start_col) {
    if (!m_terminal) return;
    
    Position pos(m_window_pos.row + screen_row, m_window_pos.col + start_col);
    m_terminal->putString(text, pos);
}

void Window::renderSyntaxHighlightedText(const std::string& text, size_t buffer_line, size_t screen_row, size_t start_col) {
    if (!m_terminal || !m_syntax_highlighter) {
        renderText(text, screen_row, start_col);
        return;
    }
    
    // Get full line for context
    std::string full_line = m_buffer->getLine(buffer_line);
    full_line = expandTabs(full_line);
    
    // Get context lines for multiline constructs
    std::vector<std::string> context_lines;
    // For now, we'll just use the current line
    context_lines.push_back(full_line);
    
    // Get syntax highlighting
    SyntaxHighlightResult highlight_result = m_syntax_highlighter->highlightLine(
        full_line, buffer_line, context_lines);
    
    Position base_pos(m_window_pos.row + screen_row, m_window_pos.col + start_col);
    
    // If no tokens, render as plain text
    if (highlight_result.tokens.empty()) {
        m_terminal->putString(text, base_pos);
        return;
    }
    
    // Render with syntax highlighting
    size_t text_pos = 0;
    size_t visible_start = m_left_column;
    size_t visible_end = visible_start + getTextAreaWidth();
    
    for (std::vector<SyntaxToken>::const_iterator token_it = highlight_result.tokens.begin(); 
         token_it != highlight_result.tokens.end(); ++token_it) {
        const SyntaxToken& token = *token_it;
        // Skip tokens that are before our visible area
        if (token.start_pos + token.length <= visible_start) {
            continue;
        }
        
        // Stop if token is beyond our visible area
        if (token.start_pos >= visible_end) {
            break;
        }
        
        // Render any unhighlighted text before this token
        if (token.start_pos > text_pos && token.start_pos > visible_start) {
            size_t start = std::max(text_pos, visible_start) - visible_start;
            size_t end = token.start_pos - visible_start;
            if (start < text.length() && end <= text.length() && start < end) {
                std::string plain_text = text.substr(start, end - start);
                Position pos(base_pos.row, base_pos.col + start);
                m_terminal->putString(plain_text, pos);
            }
        }
        
        // Render the highlighted token
        size_t token_start = std::max(token.start_pos, visible_start) - visible_start;
        size_t token_end = std::min(token.start_pos + token.length, visible_end) - visible_start;
        
        if (token_start < text.length() && token_end <= text.length() && token_start < token_end) {
            std::string token_text = text.substr(token_start, token_end - token_start);
            Position pos(base_pos.row, base_pos.col + token_start);
            m_terminal->putStringWithColor(token_text, pos, token.color, token.bg_color);
        }
        
        text_pos = token.start_pos + token.length;
    }
    
    // Render any remaining unhighlighted text
    if (text_pos < visible_end && text_pos > visible_start) {
        size_t start = text_pos - visible_start;
        if (start < text.length()) {
            std::string remaining_text = text.substr(start);
            Position pos(base_pos.row, base_pos.col + start);
            m_terminal->putString(remaining_text, pos);
        }
    }
}

} // namespace subzero