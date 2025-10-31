#include "buffer.h"
#include <fstream>
#include <algorithm>

namespace subzero {

Buffer::Buffer() 
    : m_modified(false)
    , m_readonly(false)
    , m_cursor(0, 0)
    , m_undo_index(0)
{
    m_lines.push_back(""); // Always have at least one line
}

Buffer::Buffer(const std::string& filename) 
    : m_modified(false)
    , m_readonly(false)
    , m_cursor(0, 0)
    , m_undo_index(0)
{
    m_lines.push_back(""); // Always have at least one line
    loadFromFile(filename);
}

bool Buffer::loadFromFile(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    m_filename = filename;
    return loadFromStream(file);
}

bool Buffer::loadFromStream(std::istream& stream) {
    m_lines.clear();
    m_cursor = BufferPosition(0, 0);
    m_modified = false;
    m_undo_stack.clear();
    m_undo_index = 0;
    
    std::string line;
    while (std::getline(stream, line)) {
        // Remove Windows line endings if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        m_lines.push_back(line);
    }
    
    // Ensure we have at least one line
    if (m_lines.empty()) {
        m_lines.push_back("");
    }
    
    return true;
}

bool Buffer::saveToFile(const std::string& filename) {
    std::string target_file = filename.empty() ? m_filename : filename;
    if (target_file.empty()) {
        return false;
    }
    
    std::ofstream file(target_file.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    for (size_t i = 0; i < m_lines.size(); ++i) {
        file << m_lines[i];
        if (i < m_lines.size() - 1) {
            file << '\n';
        }
    }
    
    if (!filename.empty()) {
        m_filename = filename;
    }
    m_modified = false;
    return true;
}

const std::string& Buffer::getLine(size_t line_num) const {
    static const std::string empty_line;
    if (line_num >= m_lines.size()) {
        return empty_line;
    }
    return m_lines[line_num];
}

std::string Buffer::getLineSubstring(size_t line_num, size_t start_col, size_t length) const {
    if (line_num >= m_lines.size()) {
        return "";
    }
    
    const std::string& line = m_lines[line_num];
    return utf8::substr(line, start_col, length);
}

void Buffer::setCursor(const BufferPosition& pos) {
    m_cursor = pos;
    ensureValidCursor();
}

void Buffer::moveCursor(int delta_line, int delta_col) {
    int new_line = static_cast<int>(m_cursor.line) + delta_line;
    int new_col = static_cast<int>(m_cursor.column) + delta_col;
    
    new_line = std::max(0, std::min(new_line, static_cast<int>(m_lines.size()) - 1));
    new_col = std::max(0, new_col);
    
    m_cursor.line = static_cast<size_t>(new_line);
    m_cursor.column = static_cast<size_t>(new_col);
    ensureValidCursor();
}

bool Buffer::isValidPosition(const BufferPosition& pos) const {
    if (pos.line >= m_lines.size()) {
        return false;
    }
    
    size_t line_length = utf8::length(m_lines[pos.line]);
    return pos.column <= line_length;
}

void Buffer::insertChar(char32_t unicode_char) {
    if (m_readonly) return;
    
    // Simple UTF-8 encoding for basic characters
    std::string utf8_char;
    if (unicode_char < 0x80) {
        // ASCII character
        utf8_char = static_cast<char>(unicode_char);
    } else if (unicode_char < 0x800) {
        // 2-byte UTF-8
        utf8_char += static_cast<char>(0xC0 | (unicode_char >> 6));
        utf8_char += static_cast<char>(0x80 | (unicode_char & 0x3F));
    } else if (unicode_char < 0x10000) {
        // 3-byte UTF-8
        utf8_char += static_cast<char>(0xE0 | (unicode_char >> 12));
        utf8_char += static_cast<char>(0x80 | ((unicode_char >> 6) & 0x3F));
        utf8_char += static_cast<char>(0x80 | (unicode_char & 0x3F));
    } else {
        // 4-byte UTF-8
        utf8_char += static_cast<char>(0xF0 | (unicode_char >> 18));
        utf8_char += static_cast<char>(0x80 | ((unicode_char >> 12) & 0x3F));
        utf8_char += static_cast<char>(0x80 | ((unicode_char >> 6) & 0x3F));
        utf8_char += static_cast<char>(0x80 | (unicode_char & 0x3F));
    }
    
    insertString(utf8_char);
}

void Buffer::insertString(const std::string& utf8_str) {
    if (m_readonly || utf8_str.empty()) return;
    
    std::string& current_line = m_lines[m_cursor.line];
    size_t byte_pos = utf8::charToByte(current_line, m_cursor.column);
    
    current_line.insert(byte_pos, utf8_str);
    m_cursor.column += utf8::length(utf8_str);
    
    setModified();
}

void Buffer::deleteChar() {
    if (m_readonly) return;
    
    std::string& current_line = m_lines[m_cursor.line];
    size_t line_length = utf8::length(current_line);
    
    if (m_cursor.column < line_length) {
        // Delete character at cursor
        size_t byte_pos = utf8::charToByte(current_line, m_cursor.column);
        size_t char_bytes = utf8::charByteLength(current_line, byte_pos);
        current_line.erase(byte_pos, char_bytes);
        setModified();
    } else if (m_cursor.line < m_lines.size() - 1) {
        // Join with next line
        current_line += m_lines[m_cursor.line + 1];
        m_lines.erase(m_lines.begin() + m_cursor.line + 1);
        setModified();
    }
}

void Buffer::deleteCharBefore() {
    if (m_readonly) return;
    
    if (m_cursor.column > 0) {
        // Delete character before cursor
        m_cursor.column--;
        deleteChar();
    } else if (m_cursor.line > 0) {
        // Join with previous line
        size_t prev_line_length = utf8::length(m_lines[m_cursor.line - 1]);
        m_lines[m_cursor.line - 1] += m_lines[m_cursor.line];
        m_lines.erase(m_lines.begin() + m_cursor.line);
        m_cursor.line--;
        m_cursor.column = prev_line_length;
        setModified();
    }
}

void Buffer::deleteLine() {
    if (m_readonly) return;
    
    if (m_lines.size() > 1) {
        m_lines.erase(m_lines.begin() + m_cursor.line);
        if (m_cursor.line >= m_lines.size()) {
            m_cursor.line = m_lines.size() - 1;
        }
    } else {
        m_lines[0].clear();
    }
    
    m_cursor.column = 0;
    setModified();
}

void Buffer::insertLine() {
    if (m_readonly) return;
    
    m_lines.insert(m_lines.begin() + m_cursor.line, "");
    m_cursor.column = 0;
    setModified();
}

void Buffer::insertLineAfter() {
    if (m_readonly) return;
    
    m_lines.insert(m_lines.begin() + m_cursor.line + 1, "");
    m_cursor.line++;
    m_cursor.column = 0;
    setModified();
}

void Buffer::joinLines() {
    if (m_readonly || m_cursor.line >= m_lines.size() - 1) return;
    
    std::string& current_line = m_lines[m_cursor.line];
    const std::string& next_line = m_lines[m_cursor.line + 1];
    
    // Add space if both lines have content
    if (!current_line.empty() && !next_line.empty()) {
        current_line += " ";
    }
    current_line += next_line;
    
    m_lines.erase(m_lines.begin() + m_cursor.line + 1);
    setModified();
}

void Buffer::splitLine() {
    if (m_readonly) return;
    
    std::string& current_line = m_lines[m_cursor.line];
    size_t byte_pos = utf8::charToByte(current_line, m_cursor.column);
    
    std::string new_line = current_line.substr(byte_pos);
    current_line = current_line.substr(0, byte_pos);
    
    m_lines.insert(m_lines.begin() + m_cursor.line + 1, new_line);
    m_cursor.line++;
    m_cursor.column = 0;
    setModified();
}

BufferPosition Buffer::getNextWord() const {
    BufferPosition pos = m_cursor;
    
    if (pos.line >= m_lines.size()) {
        return pos;
    }
    
    const std::string& line = m_lines[pos.line];
    size_t line_length = utf8::length(line);
    
    // Skip current word
    while (pos.column < line_length) {
        std::string ch = utf8::charAt(line, pos.column);
        if (ch.empty()) break;
        
        // Simple ASCII check for word characters
        char32_t unicode_char = static_cast<unsigned char>(ch[0]);
        if (ch.length() == 1) {
            // ASCII character
            if (!isWordChar(unicode_char)) break;
        }
        pos.column++;
    }
    
    // Skip whitespace
    while (pos.column < line_length) {
        std::string ch = utf8::charAt(line, pos.column);
        if (ch.empty() || (ch[0] != ' ' && ch[0] != '\t')) break;
        pos.column++;
    }
    
    return pos;
}

BufferPosition Buffer::getPreviousWord() const {
    BufferPosition pos = m_cursor;
    
    if (pos.column > 0) {
        pos.column--;
    } else if (pos.line > 0) {
        pos.line--;
        pos.column = utf8::length(m_lines[pos.line]);
    }
    
    return pos;
}

BufferPosition Buffer::getLineBegin() const {
    return BufferPosition(m_cursor.line, 0);
}

BufferPosition Buffer::getLineEnd() const {
    size_t line_length = utf8::length(m_lines[m_cursor.line]);
    return BufferPosition(m_cursor.line, line_length);
}

BufferPosition Buffer::getBufferBegin() const {
    return BufferPosition(0, 0);
}

BufferPosition Buffer::getBufferEnd() const {
    if (m_lines.empty()) {
        return BufferPosition(0, 0);
    }
    
    size_t last_line = m_lines.size() - 1;
    size_t last_col = utf8::length(m_lines[last_line]);
    return BufferPosition(last_line, last_col);
}

void Buffer::clear() {
    m_lines.clear();
    m_lines.push_back("");
    m_cursor = BufferPosition(0, 0);
    m_filename.clear();
    m_modified = false;
    m_undo_stack.clear();
    m_undo_index = 0;
}

void Buffer::ensureValidCursor() {
    if (m_lines.empty()) {
        m_cursor = BufferPosition(0, 0);
        return;
    }
    
    // Ensure line is valid
    if (m_cursor.line >= m_lines.size()) {
        m_cursor.line = m_lines.size() - 1;
    }
    
    // Ensure column is valid
    size_t line_length = utf8::length(m_lines[m_cursor.line]);
    if (m_cursor.column > line_length) {
        m_cursor.column = line_length;
    }
}

size_t Buffer::getLineLength(size_t line_num) const {
    if (line_num >= m_lines.size()) {
        return 0;
    }
    return utf8::length(m_lines[line_num]);
}

bool Buffer::isWordChar(char32_t ch) const {
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '_';
}

void Buffer::undo() {
    // Simplified undo - full implementation would be more complex
    if (canUndo()) {
        m_undo_index--;
        // Apply undo operation
        setModified();
    }
}

void Buffer::redo() {
    // Simplified redo - full implementation would be more complex
    if (canRedo()) {
        m_undo_index++;
        // Apply redo operation
        setModified();
    }
}

std::string Buffer::yankLine() {
    if (m_cursor.line < m_lines.size()) {
        return m_lines[m_cursor.line];
    }
    return "";
}

void Buffer::pasteAfter(const std::string& text) {
    if (m_readonly || text.empty()) return;
    
    // For now, treat all paste as line paste
    m_lines.insert(m_lines.begin() + m_cursor.line + 1, text);
    m_cursor.line++;
    m_cursor.column = 0;
    setModified();
}

void Buffer::pasteBefore(const std::string& text) {
    if (m_readonly || text.empty()) return;
    
    // For now, treat all paste as line paste
    m_lines.insert(m_lines.begin() + m_cursor.line, text);
    m_cursor.column = 0;
    setModified();
}

void Buffer::addUndoEntry(const UndoEntry& entry) {
    // Remove any redo entries
    m_undo_stack.erase(m_undo_stack.begin() + m_undo_index, m_undo_stack.end());
    
    // Add new entry
    m_undo_stack.push_back(entry);
    m_undo_index = m_undo_stack.size();
    
    // Limit undo stack size
    const size_t MAX_UNDO_ENTRIES = 1000;
    if (m_undo_stack.size() > MAX_UNDO_ENTRIES) {
        m_undo_stack.erase(m_undo_stack.begin());
        m_undo_index--;
    }
}

} // namespace subzero