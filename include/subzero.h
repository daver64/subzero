#pragma once

#include <string>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <memory>

// Terminal abstraction
#include "terminal_types.h"
#include "terminal.h"
#include "terminal_factory.h"
#include "utf8_utils.h"

// Editor components
#include "buffer.h"
#include "window.h"
#include "editor.h"
#include "syntax_highlighter.h"
#include "syntax_highlighter_manager.h"
#include "cpp_syntax_highlighter.h"

// Platform-specific implementations (conditionally included)
#ifdef LINUX_PLATFORM
#include "ncurses_terminal.h"
#elif defined(WINDOWS_PLATFORM)
#include "win_console_terminal.h"
#endif

