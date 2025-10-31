#pragma once
#include "terminal_types.h"

namespace subzero {

class ITerminal {
public:
    virtual ~ITerminal() = default;
    
    // Initialization and cleanup
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
    
    // Screen management
    virtual TerminalSize getSize() const = 0;
    virtual void clear() = 0;
    virtual void refresh() = 0;
    
    // Cursor management
    virtual void setCursor(const Position& pos) = 0;
    virtual Position getCursor() const = 0;
    virtual void showCursor(bool visible) = 0;
    
    // Text output
    virtual void putChar(const std::string& utf8_char, const Position& pos) = 0;
    virtual void putString(const std::string& utf8_str, const Position& pos) = 0;
    virtual void putStringWithColor(const std::string& utf8_str, const Position& pos, 
                                   Color::Value fg, Color::Value bg = Color::BLACK) = 0;
    
    // Input
    virtual KeyPress getKey() = 0;       // Blocking
    virtual bool hasInput() = 0;         // Non-blocking check
    
    // Attributes and colors
    virtual void setColors(Color::Value fg, Color::Value bg) = 0;
    virtual void resetAttributes() = 0;
    
    // Raw mode control
    virtual void enableRawMode() = 0;
    virtual void disableRawMode() = 0;
    virtual bool isRawMode() const = 0;
    
    // Error handling
    virtual std::string getLastError() const = 0;
};

} // namespace subzero