#pragma once
#include "terminal.h"
#include "compat.h"

namespace subzero {

class TerminalFactory {
public:
    // Create a terminal instance for the current platform
    static std::unique_ptr<ITerminal> create();
    
    // Get platform name for debugging
    static std::string getPlatformName();
    
private:
    TerminalFactory(); // Static class - private constructor
};

} // namespace subzero