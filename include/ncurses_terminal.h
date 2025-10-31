#pragma once
#include "terminal.h"

#ifdef LINUX_PLATFORM
#include <ncurses.h>
#include <locale.h>

namespace subzero {

class NcursesTerminal : public ITerminal {
private:
    bool m_initialized;
    bool m_raw_mode;
    std::string m_last_error;
    
    // Color pair management
    static constexpr int MAX_COLOR_PAIRS = 64;
    int m_next_color_pair;
    
    // Helper methods
    int getColorPair(Color::Value fg, Color::Value bg);
    int mapColor(Color::Value color);
    Key mapNcursesKey(int ch);
    
public:
    NcursesTerminal();
    virtual ~NcursesTerminal();
    
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

#endif // LINUX_PLATFORM