#include "ncurses_terminal.h"

#ifdef LINUX_PLATFORM
#include "utf8_utils.h"
#include <cstring>

namespace subzero {

NcursesTerminal::NcursesTerminal() 
    : m_initialized(false)
    , m_raw_mode(false)
    , m_next_color_pair(1)
{
}

NcursesTerminal::~NcursesTerminal() {
    shutdown();
}

bool NcursesTerminal::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Set locale for UTF-8 support
    setlocale(LC_ALL, "");
    
    // Initialize ncurses
    if (!initscr()) {
        m_last_error = "Failed to initialize ncurses";
        return false;
    }
    
    // Enable UTF-8 input/output
    if (start_color() == ERR) {
        m_last_error = "Terminal does not support colors";
        endwin();
        return false;
    }
    
    // Configure ncurses
    noecho();           // Don't echo keys
    keypad(stdscr, TRUE); // Enable function keys
    nodelay(stdscr, FALSE); // Blocking input by default
    
    // Initialize basic colors
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    
    m_initialized = true;
    return true;
}

void NcursesTerminal::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    disableRawMode();
    endwin();
    m_initialized = false;
}

bool NcursesTerminal::isInitialized() const {
    return m_initialized;
}

TerminalSize NcursesTerminal::getSize() const {
    if (!m_initialized) {
        return TerminalSize(0, 0);
    }
    
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    return TerminalSize(rows, cols);
}

void NcursesTerminal::clear() {
    if (!m_initialized) return;
    ::clear();
}

void NcursesTerminal::refresh() {
    if (!m_initialized) return;
    ::refresh();
}

void NcursesTerminal::setCursor(const Position& pos) {
    if (!m_initialized) return;
    move(pos.row, pos.col);
}

Position NcursesTerminal::getCursor() const {
    if (!m_initialized) return Position(0, 0);
    
    int row, col;
    getyx(stdscr, row, col);
    return Position(row, col);
}

void NcursesTerminal::showCursor(bool visible) {
    if (!m_initialized) return;
    curs_set(visible ? 1 : 0);
}

void NcursesTerminal::putChar(const std::string& utf8_char, const Position& pos) {
    if (!m_initialized || utf8_char.empty()) return;
    
    move(pos.row, pos.col);
    addstr(utf8_char.c_str());
}

void NcursesTerminal::putString(const std::string& utf8_str, const Position& pos) {
    if (!m_initialized) return;
    
    move(pos.row, pos.col);
    addstr(utf8_str.c_str());
}

void NcursesTerminal::putStringWithColor(const std::string& utf8_str, const Position& pos, 
                                        Color::Value fg, Color::Value bg) {
    if (!m_initialized) return;
    
    int color_pair = getColorPair(fg, bg);
    move(pos.row, pos.col);
    attron(COLOR_PAIR(color_pair));
    addstr(utf8_str.c_str());
    attroff(COLOR_PAIR(color_pair));
}

KeyPress NcursesTerminal::getKey() {
    if (!m_initialized) return KeyPress(Key::UNKNOWN);
    
    int ch = getch();
    
    // Handle special keys
    if (ch >= KEY_MIN) {
        Key key = mapNcursesKey(ch);
        return KeyPress(key);
    }
    
    // Handle control characters
    if (ch < 32) {
        if (ch == 27) return KeyPress(Key::ESCAPE);
        if (ch == 8 || ch == 127) return KeyPress(Key::BACKSPACE);
        if (ch == 9) return KeyPress(Key::TAB);
        if (ch == 10 || ch == 13) return KeyPress(Key::ENTER);
        
        // Ctrl+A through Ctrl+Z
        if (ch >= 1 && ch <= 26) {
            return KeyPress(static_cast<Key>(static_cast<int>(Key::CTRL_A) + ch - 1));
        }
        
        return KeyPress(Key::UNKNOWN);
    }
    
    // Handle UTF-8 characters
    std::string utf8_char;
    utf8_char += static_cast<char>(ch);
    
    // If it's the start of a multi-byte UTF-8 sequence, read more bytes
    if ((ch & 0x80) != 0) {
        size_t expected_bytes = 0;
        if ((ch & 0xE0) == 0xC0) expected_bytes = 2;
        else if ((ch & 0xF0) == 0xE0) expected_bytes = 3;
        else if ((ch & 0xF8) == 0xF0) expected_bytes = 4;
        
        for (size_t i = 1; i < expected_bytes; ++i) {
            int next_ch = getch();
            if (next_ch == ERR) break;
            utf8_char += static_cast<char>(next_ch);
        }
    }
    
    return KeyPress(utf8_char);
}

bool NcursesTerminal::hasInput() {
    if (!m_initialized) return false;
    
    nodelay(stdscr, TRUE);
    int ch = getch();
    nodelay(stdscr, FALSE);
    
    if (ch != ERR) {
        ungetch(ch);
        return true;
    }
    
    return false;
}

void NcursesTerminal::setColors(Color::Value fg, Color::Value bg) {
    if (!m_initialized) return;
    
    int color_pair = getColorPair(fg, bg);
    attron(COLOR_PAIR(color_pair));
}

void NcursesTerminal::resetAttributes() {
    if (!m_initialized) return;
    attrset(A_NORMAL);
}

void NcursesTerminal::enableRawMode() {
    if (!m_initialized) return;
    raw();
    m_raw_mode = true;
}

void NcursesTerminal::disableRawMode() {
    if (!m_initialized) return;
    noraw();
    m_raw_mode = false;
}

bool NcursesTerminal::isRawMode() const {
    return m_raw_mode;
}

std::string NcursesTerminal::getLastError() const {
    return m_last_error;
}

int NcursesTerminal::getColorPair(Color::Value fg, Color::Value bg) {
    // Simple color pair management - could be improved with caching
    if (m_next_color_pair >= MAX_COLOR_PAIRS) {
        return 1; // Fallback to default
    }
    
    int fg_color = mapColor(fg);
    int bg_color = mapColor(bg);
    
    init_pair(m_next_color_pair, fg_color, bg_color);
    return m_next_color_pair++;
}

int NcursesTerminal::mapColor(Color::Value color) {
    switch (color) {
        case Color::BLACK: return COLOR_BLACK;
        case Color::RED: return COLOR_RED;
        case Color::GREEN: return COLOR_GREEN;
        case Color::YELLOW: return COLOR_YELLOW;
        case Color::BLUE: return COLOR_BLUE;
        case Color::MAGENTA: return COLOR_MAGENTA;
        case Color::CYAN: return COLOR_CYAN;
        case Color::WHITE: return COLOR_WHITE;
        // Bright colors - ncurses doesn't have direct support, use regular colors
        case Color::BRIGHT_BLACK: return COLOR_BLACK;
        case Color::BRIGHT_RED: return COLOR_RED;
        case Color::BRIGHT_GREEN: return COLOR_GREEN;
        case Color::BRIGHT_YELLOW: return COLOR_YELLOW;
        case Color::BRIGHT_BLUE: return COLOR_BLUE;
        case Color::BRIGHT_MAGENTA: return COLOR_MAGENTA;
        case Color::BRIGHT_CYAN: return COLOR_CYAN;
        case Color::BRIGHT_WHITE: return COLOR_WHITE;
        default: return COLOR_WHITE;
    }
}

Key NcursesTerminal::mapNcursesKey(int ch) {
    switch (ch) {
        case KEY_UP: return Key::ARROW_UP;
        case KEY_DOWN: return Key::ARROW_DOWN;
        case KEY_LEFT: return Key::ARROW_LEFT;
        case KEY_RIGHT: return Key::ARROW_RIGHT;
        case KEY_HOME: return Key::HOME;
        case KEY_END: return Key::END;
        case KEY_PPAGE: return Key::PAGE_UP;
        case KEY_NPAGE: return Key::PAGE_DOWN;
        case KEY_BACKSPACE: return Key::BACKSPACE;
        case KEY_DC: return Key::DELETE;
        case KEY_F(1): return Key::F1;
        case KEY_F(2): return Key::F2;
        case KEY_F(3): return Key::F3;
        case KEY_F(4): return Key::F4;
        case KEY_F(5): return Key::F5;
        case KEY_F(6): return Key::F6;
        case KEY_F(7): return Key::F7;
        case KEY_F(8): return Key::F8;
        case KEY_F(9): return Key::F9;
        case KEY_F(10): return Key::F10;
        case KEY_F(11): return Key::F11;
        case KEY_F(12): return Key::F12;
        default: return Key::UNKNOWN;
    }
}

} // namespace subzero

#endif // LINUX_PLATFORM