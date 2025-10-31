#pragma once
#include "compat.h"
#include "utf8_utils.h"
#include <vector>
#include <string>
#include <fstream>

namespace subzero {

struct BufferPosition {
    size_t line;
    size_t column;  // Character position, not byte position
    
    BufferPosition(size_t l = 0, size_t c = 0) : line(l), column(c) {}
    
    bool operator==(const BufferPosition& other) const {
        return line == other.line && column == other.column;
    }
    
    bool operator!=(const BufferPosition& other) const {
        return !(*this == other);
    }
};

class Buffer {
private:
    std::vector<std::string> m_lines;
    std::string m_filename;
    bool m_modified;
    bool m_readonly;
    BufferPosition m_cursor;
    
    // Undo/redo support (simplified for now)
    struct UndoEntry {
        enum Type { INSERT_CHAR, DELETE_CHAR, INSERT_LINE, DELETE_LINE };
        Type type;
        BufferPosition position;
        std::string data;
    };
    std::vector<UndoEntry> m_undo_stack;
    size_t m_undo_index;
    
public:
    Buffer();
    explicit Buffer(const std::string& filename);
    
    // File operations
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename = "");
    bool isModified() const { return m_modified; }
    bool isReadonly() const { return m_readonly; }
    const std::string& getFilename() const { return m_filename; }
    void setFilename(const std::string& filename) { m_filename = filename; }
    
    // Content access
    size_t getLineCount() const { return m_lines.size(); }
    const std::string& getLine(size_t line_num) const;
    std::string getLineSubstring(size_t line_num, size_t start_col, size_t length = std::string::npos) const;
    
    // Cursor operations
    const BufferPosition& getCursor() const { return m_cursor; }
    void setCursor(const BufferPosition& pos);
    void moveCursor(int delta_line, int delta_col);
    bool isValidPosition(const BufferPosition& pos) const;
    
    // Text editing operations
    void insertChar(char32_t unicode_char);
    void insertString(const std::string& utf8_str);
    void deleteChar();           // Delete character at cursor
    void deleteCharBefore();     // Backspace
    void deleteLine();
    void insertLine();
    void insertLineAfter();
    
    // Line operations
    void joinLines();            // Join current line with next
    void splitLine();            // Split line at cursor
    
    // Vi-specific operations
    void deleteWord();
    void deleteToEndOfLine();
    void deleteToBeginningOfLine();
    std::string yankLine();      // Copy line
    std::string yankWord();      // Copy word
    void pasteBefore(const std::string& text);
    void pasteAfter(const std::string& text);
    
    // Search operations
    BufferPosition findNext(const std::string& pattern, const BufferPosition& start) const;
    BufferPosition findPrevious(const std::string& pattern, const BufferPosition& start) const;
    
    // Word/character navigation
    BufferPosition getNextWord() const;
    BufferPosition getPreviousWord() const;
    BufferPosition getLineBegin() const;
    BufferPosition getLineEnd() const;
    BufferPosition getBufferBegin() const;
    BufferPosition getBufferEnd() const;
    
    // Undo/redo
    bool canUndo() const { return m_undo_index > 0; }
    bool canRedo() const { return m_undo_index < m_undo_stack.size(); }
    void undo();
    void redo();
    
    // Utility
    void clear();
    bool isEmpty() const { return m_lines.empty() || (m_lines.size() == 1 && m_lines[0].empty()); }
    
private:
    void ensureValidCursor();
    void addUndoEntry(const UndoEntry& entry);
    void setModified(bool modified = true) { m_modified = modified; }
    size_t getLineLength(size_t line_num) const;
    bool isWordChar(char32_t ch) const;
};

} // namespace subzero