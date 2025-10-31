#include "ncurses_terminal.h"

#if defined(LINUX_PLATFORM) || defined(MINTOS_PLATFORM)
#include "utf8_utils.h"
#include <cstring>
#include <cstdlib>  // For setenv
#include <cstdio>   // For printf

// Debug output control - disable debug output on MiNTOS by default
#ifdef MINTOS_PLATFORM
    // On Atari, minimize debug output to avoid screen clutter
    #define DEBUG_PRINT(fmt, ...) do {} while(0)
#else
    // On other platforms, allow debug output
    #define DEBUG_PRINT(fmt, ...) do { printf(fmt, ##__VA_ARGS__); fflush(stdout); } while(0)
#endif

namespace subzero {

NcursesTerminal::NcursesTerminal() 
    : m_initialized(false)
    , m_raw_mode(false)
    , m_next_color_pair(1)
{
    DEBUG_PRINT("NcursesTerminal constructor called\n");
}

NcursesTerminal::~NcursesTerminal() {
    shutdown();
}

bool NcursesTerminal::initialize() {
    DEBUG_PRINT("NcursesTerminal::initialize() called\n");
    
    if (m_initialized) {
        DEBUG_PRINT("Already initialized, returning true\n");
        return true;
    }
    
    DEBUG_PRINT("Setting locale...\n");
    // Set locale for UTF-8 support
    setlocale(LC_ALL, "");
    
    // Check and set terminal environment for embedded systems
    const char* term_env = getenv("TERM");
    DEBUG_PRINT("Current TERM=%s\n", term_env ? term_env : "NULL");
    
    if (!term_env || strlen(term_env) == 0) {
        DEBUG_PRINT("TERM not set, setting default...\n");
        // Set a basic terminal type for systems without TERM set
        #ifdef MINTOS_PLATFORM
        setenv("TERM", "tw100-m", 1);  // Atari TW100 terminal
        DEBUG_PRINT("Set TERM=tw100-m for MiNTOS\n");
        #else
        setenv("TERM", "xterm", 1); // Default to xterm
        DEBUG_PRINT("Set TERM=xterm for other platforms\n");
        #endif
    }
    
    DEBUG_PRINT("Attempting newterm()...\n");
    
    // Try to initialize ncurses with error checking
    SCREEN* screen = newterm(NULL, stdout, stdin);
    bool using_initscr = false;
    
    if (!screen) {
        DEBUG_PRINT("newterm() failed, trying fallbacks...\n");
        // Fallback: try with basic terminal types, including Atari-specific ones
        const char* fallback_terms[] = {"tw100-m", "tw100", "tw52", "ansi", "vt100", "vt52", "dumb", NULL};
        for (int i = 0; fallback_terms[i] != NULL; i++) {
            DEBUG_PRINT("Trying terminal type: %s\n", fallback_terms[i]);
            screen = newterm(fallback_terms[i], stdout, stdin);
            if (screen) {
                DEBUG_PRINT("Success with terminal type: %s\n", fallback_terms[i]);
                break;
            }
        }
        
        if (!screen) {
            DEBUG_PRINT("All newterm() attempts failed, trying initscr() fallback...\n");
            
            // Last resort: try initscr() which doesn't require specific terminfo
            WINDOW* win = initscr();
            if (win) {
                DEBUG_PRINT("Success with initscr() fallback - using basic terminal support\n");
                using_initscr = true;
            } else {
                printf("initscr() also failed - no terminfo database available\n");
                fflush(stdout);
                m_last_error = "Failed to initialize terminal - no compatible terminal type found";
                return false;
            }
        }
    } else {
        DEBUG_PRINT("newterm() succeeded with default terminal\n");
    }
    
    // Set the screen as current (only if we're not using initscr)
    if (!using_initscr && screen) {
        set_term(screen);
    }
    
    // Check if colors are supported before requiring them
    bool color_support = has_colors();
    if (color_support) {
        if (start_color() == ERR) {
            // Colors failed but terminal works - continue without colors
            color_support = false;
        }
    }
    
    // Configure ncurses
    noecho();           // Don't echo keys
    keypad(stdscr, TRUE); // Enable function keys
    nodelay(stdscr, FALSE); // Blocking input by default
    
    // Clear screen to remove any debug output from startup
    clear();
    refresh();
    
    // Initialize basic colors only if supported
    if (color_support) {
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
    }
    
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
    // Ensure we're using default colors for plain text
    resetAttributes();
    addstr(utf8_str.c_str());
}

void NcursesTerminal::putStringWithColor(const std::string& utf8_str, const Position& pos, 
                                        Color::Value fg, Color::Value bg) {
    if (!m_initialized) return;
    
    int color_pair = getColorPair(fg, bg);
    move(pos.row, pos.col);
    
    // Clear any existing attributes first, then set color
    resetAttributes();
    attron(COLOR_PAIR(color_pair));
    addstr(utf8_str.c_str());
    attroff(COLOR_PAIR(color_pair));
    // Reset to normal attributes
    resetAttributes();
}

KeyPress NcursesTerminal::getKey() {
    if (!m_initialized) return KeyPress(UNKNOWN);
    
    int ch = getch();
    
    // Handle special keys
    if (ch >= KEY_MIN) {
        Key key = mapNcursesKey(ch);
        return KeyPress(key);
    }
    
    // Handle control characters
    if (ch < 32) {
        if (ch == 27) return KeyPress(ESCAPE);
        if (ch == 8 || ch == 127) return KeyPress(BACKSPACE);
        if (ch == 9) return KeyPress(TAB);
        if (ch == 10 || ch == 13) return KeyPress(ENTER);
        
        // Ctrl+A through Ctrl+Z
        if (ch >= 1 && ch <= 26) {
            return KeyPress(static_cast<Key>(static_cast<int>(CTRL_A) + ch - 1));
        }
        
        return KeyPress(UNKNOWN);
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
    // If terminal doesn't support colors, return default
    if (!has_colors()) {
        return 0; // Default color pair
    }
    
    // Create a unique key for this color combination
    int key = (static_cast<int>(fg) << 8) | static_cast<int>(bg);
    
    // Check if we already have this color pair
    std::map<int, int>::iterator it = m_color_pair_cache.find(key);
    if (it != m_color_pair_cache.end()) {
        return it->second;  // Return existing color pair
    }
    
    // Create new color pair if we have room
    if (m_next_color_pair >= MAX_COLOR_PAIRS) {
        return 1; // Fallback to default
    }
    
    int fg_color = mapColor(fg);
    int bg_color = mapColor(bg);
    
    int pair_id = m_next_color_pair++;
    init_pair(pair_id, fg_color, bg_color);
    
    // Cache this color pair for future use
    m_color_pair_cache[key] = pair_id;
    
    return pair_id;
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
        case KEY_UP: return ARROW_UP;
        case KEY_DOWN: return ARROW_DOWN;
        case KEY_LEFT: return ARROW_LEFT;
        case KEY_RIGHT: return ARROW_RIGHT;
        case KEY_HOME: return HOME;
        case KEY_END: return END;
        case KEY_PPAGE: return PAGE_UP;
        case KEY_NPAGE: return PAGE_DOWN;
        case KEY_BACKSPACE: return BACKSPACE;
        case KEY_DC: return DELETE;
        case KEY_F(1): return F1;
        case KEY_F(2): return F2;
        case KEY_F(3): return F3;
        case KEY_F(4): return F4;
        case KEY_F(5): return F5;
        case KEY_F(6): return F6;
        case KEY_F(7): return F7;
        case KEY_F(8): return F8;
        case KEY_F(9): return F9;
        case KEY_F(10): return F10;
        case KEY_F(11): return F11;
        case KEY_F(12): return F12;
        default: return UNKNOWN;
    }
}

} // namespace subzero

#endif // LINUX_PLATFORM