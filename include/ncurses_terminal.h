#pragma once
#include "terminal.h"
#include <map>

#if defined(LINUX_PLATFORM) || defined(MINTOS_PLATFORM)
#include <ncurses.h>
#include <locale.h>

namespace subzero {

class NcursesTerminal : public ITerminal {
private:
    bool m_initialized;
    bool m_raw_mode;
    std::string m_last_error;
    
    // Color pair management
    static const int MAX_COLOR_PAIRS = 64;
    int m_next_color_pair;
    std::map<int, int> m_color_pair_cache;  // Maps color combination to pair ID
    
    // Helper methods
    int getColorPair(Color::Value fg, Color::Value bg);
    int mapColor(Color::Value color);
    Key mapNcursesKey(int ch);
    
public:
    NcursesTerminal();
    virtual ~NcursesTerminal();
    
    // ITerminal interface
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    TerminalSize getSize() const;
    void clear();
    void refresh();
    
    void setCursor(const Position& pos);
    Position getCursor() const;
    void showCursor(bool visible);
    
    void putChar(const std::string& utf8_char, const Position& pos);
    void putString(const std::string& utf8_str, const Position& pos);
    void putStringWithColor(const std::string& utf8_str, const Position& pos, 
                           Color::Value fg, Color::Value bg = Color::BLACK);
    
    KeyPress getKey();
    bool hasInput();
    
    void setColors(Color::Value fg, Color::Value bg);
    void resetAttributes();
    
    void enableRawMode();
    void disableRawMode();
    bool isRawMode() const;
    
    std::string getLastError() const;
};

} // namespace subzero

#endif // LINUX_PLATFORM