#pragma once
#include "terminal.h"
#include <memory>

namespace subzero {

class TerminalFactory {
public:
    // Create a terminal instance for the current platform
    static std::unique_ptr<ITerminal> create();
    
    // Get platform name for debugging
    static std::string getPlatformName();
    
private:
    TerminalFactory() = delete; // Static class
};

} // namespace subzero