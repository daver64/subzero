#pragma once
#include "terminal.h"

#ifdef WINDOWS_PLATFORM
#include <windows.h>
#include <io.h>
#include <fcntl.h>

namespace subzero {

class WinConsoleTerminal : public ITerminal {
private:
    bool m_initialized;
    bool m_raw_mode;
    std::string m_last_error;
    
    HANDLE m_stdin_handle;
    HANDLE m_stdout_handle;
    DWORD m_original_input_mode;
    DWORD m_original_output_mode;
    UINT m_original_cp;
    UINT m_original_output_cp;
    
    // Console screen buffer
    CONSOLE_SCREEN_BUFFER_INFO m_screen_info;
    
    // Helper methods
    WORD mapColor(Color::Value fg, Color::Value bg);
    Key mapWindowsKey(WORD vk_code, WCHAR unicode_char);
    void setConsoleColors(Color::Value fg, Color::Value bg);
    
public:
    WinConsoleTerminal();
    virtual ~WinConsoleTerminal();
    
    // ITerminal interface
    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;
    
    TerminalSize getSize() const override;
    void clear() override;
    void refresh() override;
    
    void setCursor(const Position& pos) override;
    Position getCursor() const override;
    void showCursor(bool visible) override;
    
    void putChar(const std::string& utf8_char, const Position& pos) override;
    void putString(const std::string& utf8_str, const Position& pos) override;
    void putStringWithColor(const std::string& utf8_str, const Position& pos, 
                           Color::Value fg, Color::Value bg = Color::BLACK) override;
    
    KeyPress getKey() override;
    bool hasInput() override;
    
    void setColors(Color::Value fg, Color::Value bg) override;
    void resetAttributes() override;
    
    void enableRawMode() override;
    void disableRawMode() override;
    bool isRawMode() const override;
    
    std::string getLastError() const override;
};

} // namespace subzero

#endif // WINDOWS_PLATFORM