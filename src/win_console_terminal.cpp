#include "win_console_terminal.h"

#ifdef WINDOWS_PLATFORM
#include "utf8_utils.h"
#include <iostream>
#include <codecvt>
#include <locale>

namespace subzero {

WinConsoleTerminal::WinConsoleTerminal() 
    : m_initialized(false)
    , m_raw_mode(false)
    , m_stdin_handle(INVALID_HANDLE_VALUE)
    , m_stdout_handle(INVALID_HANDLE_VALUE)
    , m_original_input_mode(0)
    , m_original_output_mode(0)
    , m_original_cp(0)
    , m_original_output_cp(0)
{
    ZeroMemory(&m_screen_info, sizeof(m_screen_info));
}

WinConsoleTerminal::~WinConsoleTerminal() {
    shutdown();
}

bool WinConsoleTerminal::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Get console handles
    m_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    m_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (m_stdin_handle == INVALID_HANDLE_VALUE || m_stdout_handle == INVALID_HANDLE_VALUE) {
        m_last_error = "Failed to get console handles";
        return false;
    }
    
    // Save original console modes
    if (!GetConsoleMode(m_stdin_handle, &m_original_input_mode)) {
        m_last_error = "Failed to get input console mode";
        return false;
    }
    
    if (!GetConsoleMode(m_stdout_handle, &m_original_output_mode)) {
        m_last_error = "Failed to get output console mode";
        return false;
    }
    
    // Save original code pages
    m_original_cp = GetConsoleCP();
    m_original_output_cp = GetConsoleOutputCP();
    
    // Enable UTF-8 support
    if (!SetConsoleCP(CP_UTF8) || !SetConsoleOutputCP(CP_UTF8)) {
        m_last_error = "Failed to set UTF-8 code page";
        return false;
    }
    
    // Enable virtual terminal processing for ANSI escape sequences (Windows 10+)
    DWORD output_mode = m_original_output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(m_stdout_handle, output_mode);
    
    // Configure input mode
    DWORD input_mode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    if (!SetConsoleMode(m_stdin_handle, input_mode)) {
        m_last_error = "Failed to set input console mode";
        return false;
    }
    
    // Get screen buffer info
    if (!GetConsoleScreenBufferInfo(m_stdout_handle, &m_screen_info)) {
        m_last_error = "Failed to get screen buffer info";
        return false;
    }
    
    m_initialized = true;
    return true;
}

void WinConsoleTerminal::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    disableRawMode();
    
    // Restore original console modes
    SetConsoleMode(m_stdin_handle, m_original_input_mode);
    SetConsoleMode(m_stdout_handle, m_original_output_mode);
    
    // Restore original code pages
    SetConsoleCP(m_original_cp);
    SetConsoleOutputCP(m_original_output_cp);
    
    m_initialized = false;
}

bool WinConsoleTerminal::isInitialized() const {
    return m_initialized;
}

TerminalSize WinConsoleTerminal::getSize() const {
    if (!m_initialized) {
        return TerminalSize(0, 0);
    }
    
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(m_stdout_handle, &info)) {
        int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        int cols = info.srWindow.Right - info.srWindow.Left + 1;
        return TerminalSize(rows, cols);
    }
    
    return TerminalSize(0, 0);
}

void WinConsoleTerminal::clear() {
    if (!m_initialized) return;
    
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(m_stdout_handle, &info);
    
    COORD home = {0, 0};
    DWORD written;
    DWORD size = info.dwSize.X * info.dwSize.Y;
    
    FillConsoleOutputCharacterW(m_stdout_handle, L' ', size, home, &written);
    FillConsoleOutputAttribute(m_stdout_handle, info.wAttributes, size, home, &written);
    SetConsoleCursorPosition(m_stdout_handle, home);
}

void WinConsoleTerminal::refresh() {
    if (!m_initialized) return;
    // Windows console updates immediately, no explicit refresh needed
}

void WinConsoleTerminal::setCursor(const Position& pos) {
    if (!m_initialized) return;
    
    COORD coord = {static_cast<SHORT>(pos.col), static_cast<SHORT>(pos.row)};
    SetConsoleCursorPosition(m_stdout_handle, coord);
}

Position WinConsoleTerminal::getCursor() const {
    if (!m_initialized) return Position(0, 0);
    
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(m_stdout_handle, &info)) {
        return Position(info.dwCursorPosition.Y, info.dwCursorPosition.X);
    }
    
    return Position(0, 0);
}

void WinConsoleTerminal::showCursor(bool visible) {
    if (!m_initialized) return;
    
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(m_stdout_handle, &info);
    info.bVisible = visible;
    SetConsoleCursorInfo(m_stdout_handle, &info);
}

void WinConsoleTerminal::putChar(const std::string& utf8_char, const Position& pos) {
    if (!m_initialized || utf8_char.empty()) return;
    
    setCursor(pos);
    
    // Convert UTF-8 to wide string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide_char = converter.from_bytes(utf8_char);
    
    DWORD written;
    WriteConsoleW(m_stdout_handle, wide_char.c_str(), static_cast<DWORD>(wide_char.length()), &written, NULL);
}

void WinConsoleTerminal::putString(const std::string& utf8_str, const Position& pos) {
    if (!m_initialized) return;
    
    setCursor(pos);
    
    // Convert UTF-8 to wide string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide_str = converter.from_bytes(utf8_str);
    
    DWORD written;
    WriteConsoleW(m_stdout_handle, wide_str.c_str(), static_cast<DWORD>(wide_str.length()), &written, NULL);
}

void WinConsoleTerminal::putStringWithColor(const std::string& utf8_str, const Position& pos, 
                                           Color::Value fg, Color::Value bg) {
    if (!m_initialized) return;
    
    setCursor(pos);
    setConsoleColors(fg, bg);
    putString(utf8_str, getCursor());
    resetAttributes();
}

KeyPress WinConsoleTerminal::getKey() {
    if (!m_initialized) return KeyPress(UNKNOWN);
    
    INPUT_RECORD input_record;
    DWORD events_read;
    
    while (true) {
        if (!ReadConsoleInputW(m_stdin_handle, &input_record, 1, &events_read)) {
            return KeyPress(UNKNOWN);
        }
        
        if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown) {
            KEY_EVENT_RECORD& key_event = input_record.Event.KeyEvent;
            
            // Handle special keys
            if (key_event.wVirtualKeyCode != 0) {
                Key special_key = mapWindowsKey(key_event.wVirtualKeyCode, key_event.uChar.UnicodeChar);
                if (special_key != UNKNOWN) {
                    return KeyPress(special_key);
                }
            }
            
            // Handle Unicode characters
            if (key_event.uChar.UnicodeChar != 0) {
                wchar_t wide_char = key_event.uChar.UnicodeChar;
                
                // Convert to UTF-8
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::string utf8_char = converter.to_bytes(wide_char);
                
                return KeyPress(utf8_char);
            }
        }
    }
}

bool WinConsoleTerminal::hasInput() {
    if (!m_initialized) return false;
    
    DWORD events_available;
    if (GetNumberOfConsoleInputEvents(m_stdin_handle, &events_available)) {
        return events_available > 0;
    }
    
    return false;
}

void WinConsoleTerminal::setColors(Color::Value fg, Color::Value bg) {
    if (!m_initialized) return;
    setConsoleColors(fg, bg);
}

void WinConsoleTerminal::resetAttributes() {
    if (!m_initialized) return;
    SetConsoleTextAttribute(m_stdout_handle, m_screen_info.wAttributes);
}

void WinConsoleTerminal::enableRawMode() {
    if (!m_initialized) return;
    
    DWORD input_mode = 0; // Disable all input processing
    SetConsoleMode(m_stdin_handle, input_mode);
    m_raw_mode = true;
}

void WinConsoleTerminal::disableRawMode() {
    if (!m_initialized) return;
    
    SetConsoleMode(m_stdin_handle, m_original_input_mode);
    m_raw_mode = false;
}

bool WinConsoleTerminal::isRawMode() const {
    return m_raw_mode;
}

std::string WinConsoleTerminal::getLastError() const {
    return m_last_error;
}

// Helper function to map single color values
static WORD mapSingleColor(Color::Value color) {
    switch (color) {
        case Color::BLACK: return 0;
        case Color::RED: return FOREGROUND_RED;
        case Color::GREEN: return FOREGROUND_GREEN;
        case Color::YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
        case Color::BLUE: return FOREGROUND_BLUE;
        case Color::MAGENTA: return FOREGROUND_RED | FOREGROUND_BLUE;
        case Color::CYAN: return FOREGROUND_GREEN | FOREGROUND_BLUE;
        case Color::WHITE: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case Color::BRIGHT_BLACK: return FOREGROUND_INTENSITY;
        case Color::BRIGHT_RED: return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case Color::BRIGHT_GREEN: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case Color::BRIGHT_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case Color::BRIGHT_BLUE: return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Color::BRIGHT_MAGENTA: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Color::BRIGHT_CYAN: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Color::BRIGHT_WHITE: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}

WORD WinConsoleTerminal::mapColor(Color::Value fg, Color::Value bg) {
    WORD fg_attr = mapSingleColor(fg);
    WORD bg_attr = mapSingleColor(bg) << 4; // Background colors are shifted
    
    return fg_attr | bg_attr;
}

Key WinConsoleTerminal::mapWindowsKey(WORD vk_code, WCHAR unicode_char) {
    // Handle control characters
    if (unicode_char < 32) {
        if (unicode_char == 27) return ESCAPE;
        if (unicode_char == 8) return BACKSPACE;
        if (unicode_char == 9) return TAB;
        if (unicode_char == 13) return ENTER;
        
        // Ctrl+A through Ctrl+Z
        if (unicode_char >= 1 && unicode_char <= 26) {
            return static_cast<Key>(static_cast<int>(CTRL_A) + unicode_char - 1);
        }
    }
    
    // Handle special keys
    switch (vk_code) {
        case VK_UP: return ARROW_UP;
        case VK_DOWN: return ARROW_DOWN;
        case VK_LEFT: return ARROW_LEFT;
        case VK_RIGHT: return ARROW_RIGHT;
        case VK_HOME: return HOME;
        case VK_END: return END;
        case VK_PRIOR: return PAGE_UP;
        case VK_NEXT: return PAGE_DOWN;
        case VK_DELETE: return DELETE;
        case VK_F1: return F1;
        case VK_F2: return F2;
        case VK_F3: return F3;
        case VK_F4: return F4;
        case VK_F5: return F5;
        case VK_F6: return F6;
        case VK_F7: return F7;
        case VK_F8: return F8;
        case VK_F9: return F9;
        case VK_F10: return F10;
        case VK_F11: return F11;
        case VK_F12: return F12;
        default: return UNKNOWN;
    }
}

void WinConsoleTerminal::setConsoleColors(Color::Value fg, Color::Value bg) {
    WORD attributes = mapColor(fg, bg);
    SetConsoleTextAttribute(m_stdout_handle, attributes);
}

} // namespace subzero

#endif // WINDOWS_PLATFORM