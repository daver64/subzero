#pragma once
#include <string>

namespace subzero {

struct Color {
    enum Value {
        BLACK = 0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
        BRIGHT_BLACK, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
        BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE
    };
};

struct Position {
    int row, col;
    Position(int r = 0, int c = 0) : row(r), col(c) {}
    
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

struct TerminalSize {
    int rows, cols;
    TerminalSize(int r = 0, int c = 0) : rows(r), cols(c) {}
    
    bool isValid() const {
        return rows > 0 && cols > 0;
    }
};

enum Key {
    // Printable characters will be handled separately
    ESCAPE = 256, BACKSPACE, DELETE, TAB, ENTER,
    ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT,
    HOME, END, PAGE_UP, PAGE_DOWN,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    CTRL_A, CTRL_B, CTRL_C, CTRL_D, CTRL_E, CTRL_F, CTRL_G,
    CTRL_H, CTRL_I, CTRL_J, CTRL_K, CTRL_L, CTRL_M, CTRL_N,
    CTRL_O, CTRL_P, CTRL_Q, CTRL_R, CTRL_S, CTRL_T, CTRL_U,
    CTRL_V, CTRL_W, CTRL_X, CTRL_Y, CTRL_Z,
    UNKNOWN
};

struct KeyPress {
    Key key;
    std::string utf8_char;  // For printable UTF-8 characters
    bool is_character;
    
    KeyPress(Key k) : key(k), is_character(false) {}
    KeyPress(const std::string& utf8) : key(UNKNOWN), utf8_char(utf8), is_character(true) {}
    
    bool isCharacter() const { return is_character; }
    bool isSpecialKey() const { return !is_character; }
};

} // namespace subzero