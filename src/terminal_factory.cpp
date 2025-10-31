#include "terminal_factory.h"

#ifdef LINUX_PLATFORM
#include "ncurses_terminal.h"
#elif defined(WINDOWS_PLATFORM)
#include "win_console_terminal.h"
#endif

namespace subzero {

std::unique_ptr<ITerminal> TerminalFactory::create() {
#ifdef LINUX_PLATFORM
    return std::make_unique<NcursesTerminal>();
#elif defined(WINDOWS_PLATFORM)
    return std::make_unique<WinConsoleTerminal>();
#else
    #error "Unsupported platform - please define LINUX_PLATFORM or WINDOWS_PLATFORM"
    return nullptr;
#endif
}

std::string TerminalFactory::getPlatformName() {
#ifdef LINUX_PLATFORM
    return "Linux (ncurses)";
#elif defined(WINDOWS_PLATFORM)
    return "Windows (Console API)";
#else
    return "Unknown";
#endif
}

} // namespace subzero