#include "terminal_factory.h"

#if defined(LINUX_PLATFORM) || defined(MINTOS_PLATFORM)
#include "ncurses_terminal.h"
#elif defined(WINDOWS_PLATFORM)
#include "win_console_terminal.h"
#endif

namespace subzero {

std::unique_ptr<ITerminal> TerminalFactory::create() {
#if defined(LINUX_PLATFORM) || defined(MINTOS_PLATFORM)
    return std::unique_ptr<ITerminal>(new NcursesTerminal());
#elif defined(WINDOWS_PLATFORM)
    return std::unique_ptr<ITerminal>(new WinConsoleTerminal());
#else
    #error "Unsupported platform - please define LINUX_PLATFORM, MINTOS_PLATFORM, or WINDOWS_PLATFORM"
    return std::unique_ptr<ITerminal>();
#endif
}

std::string TerminalFactory::getPlatformName() {
#ifdef LINUX_PLATFORM
    return "Linux (ncurses)";
#elif defined(MINTOS_PLATFORM)
    return "Atari MiNTOS (ncurses)";
#elif defined(WINDOWS_PLATFORM)
    return "Windows (Console API)";
#else
    return "Unknown";
#endif
}

} // namespace subzero