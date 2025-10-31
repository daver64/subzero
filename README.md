# Subzero Text Editor

A lightweight, cross-platform vi-like text editor with C++98 compatibility, full UTF-8 support, and built-in syntax highlighting.

## Features

### Core Functionality
- **Cross-platform**: Runs on Linux (ncurses), Windows (Console API), and embedded systems like Atari MiNTOS
- **C++98 Compatible**: Works with older compilers including GCC 4.6 on Atari Falcon
- **UTF-8 native**: Full international text support with proper character-aware operations
- **Vi-compatible**: Familiar vi key bindings and modal editing
- **Built-in syntax highlighting**: C/C++ and Markdown syntax highlighting without external dependencies
- **Multi-buffer support**: Work with multiple files simultaneously

### Vi Modes Implemented
- **Normal Mode**: Movement, editing commands, and navigation
- **Insert Mode**: Text insertion with UTF-8 support
- **Visual Mode**: Text selection and visual operations
- **Command Mode**: Ex commands (`:w`, `:q`, `:wq`, `:ls`, `:b`, etc.)
- **Search Mode**: Forward and backward search

### Multi-Buffer Commands
| Command | Action |
|---------|--------|
| `:ls` | List all open buffers |
| `:b N` | Switch to buffer N |
| `:bn` | Switch to next buffer |
| `:bp` | Switch to previous buffer |
| `:bd` | Close current buffer |
| `:bd!` | Force close buffer (discard changes) |

### Key Bindings

#### Normal Mode
| Key | Action |
|-----|--------|
| `h`, `j`, `k`, `l` | Move left, down, up, right |
| Arrow keys | Move cursor |
| `w`, `b` | Move word forward/backward |
| `0`, `$` | Move to line beginning/end |
| `gg`, `G` | Move to first/last line |
| `i` | Enter insert mode |
| `a` | Enter insert mode after cursor |
| `o`, `O` | Insert new line after/before |
| `x` | Delete character |
| `dd` | Delete line (supports repeat count: `3dd`) |
| `yy` | Yank (copy) line (supports repeat count: `2yy`) |
| `p`, `P` | Paste after/before cursor (supports repeat count) |
| `u` | Undo |
| `v`, `V` | Enter visual/visual line mode |
| `:` | Enter command mode |
| `/`, `?` | Search forward/backward |
| `n`, `N` | Next/previous search result |
| Numbers | Repeat count for commands (e.g., `3dd`, `5j`) |

#### Insert Mode
| Key | Action |
|-----|--------|
| `ESC` | Return to normal mode |
| `Backspace` | Delete character before cursor |
| `Delete` | Delete character at cursor |
| `Enter` | Insert new line |
| Arrow keys | Navigate while in insert mode |
| Any UTF-8 character | Insert text |

#### Command Mode
| Command | Action |
|---------|--------|
| `:q` | Quit (warns if unsaved changes) |
| `:q!` | Force quit |
| `:w` | Save file |
| `:w filename` | Save as filename |
| `:wq`, `:x` | Save and quit |
| `:e filename` | Open file for editing |
| `:e!` | Reload current file (discard changes) |

### Syntax Highlighting
- **Built-in C/C++ highlighter**: Keywords, types, strings, comments, and operators
- **Color-coded syntax**: Different colors for different language elements
- **Automatic detection**: Based on file extension (.c, .cpp, .h, .hpp, etc.)
- **No external dependencies**: All highlighting built into the executable

### Display Features
- **Line numbers**: Always visible line number display
- **Status bar**: Shows current mode, filename, cursor position, buffer info, and modification status
- **UTF-8 rendering**: Proper display of international characters
- **Syntax highlighting**: Color-coded text for supported file types
- **Viewport management**: Smooth scrolling and cursor tracking

## Building

### Prerequisites
- CMake 3.10 or higher
- **C++98 compatible compiler** with optional gnu++0x support
  - GCC 4.6+ (including Atari cross-compiler)
  - Clang 3.0+
  - MSVC 2008+
- **Linux**: ncurses library (`sudo apt install libncurses5-dev` on Ubuntu)
- **Windows**: Windows SDK (Console API)
- **Atari MiNTOS**: m68k-atari-mint cross-compiler

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd subzero

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run the editor
cd ..
./subzero [filename]
```

### Cross-compilation for Atari

```bash
# Set up cross-compilation toolchain
export CC=m68k-atari-mint-gcc
export CXX=m68k-atari-mint-g++

# Configure for Atari target
cmake -DCMAKE_SYSTEM_NAME=MiNT \
      -DCMAKE_C_COMPILER=m68k-atari-mint-gcc \
      -DCMAKE_CXX_COMPILER=m68k-atari-mint-g++ \
      -DCMAKE_FIND_ROOT_PATH=/usr/m68k-atari-mint \
      ..

make
```

**Note**: If you get linking errors about missing `-ltinfo`, the CMake build now automatically handles this by:
1. First trying to find both `ncurses` and `tinfo` libraries separately
2. If found, links both libraries properly
3. If not found, falls back to linking both `-lncurses -ltinfo` by name

If you still have issues, you can manually specify library paths:
```bash
# Manual library specification if auto-detection fails
cmake -DCMAKE_SYSTEM_NAME=MiNT \
      -DCMAKE_C_COMPILER=m68k-atari-mint-gcc \
      -DCMAKE_CXX_COMPILER=m68k-atari-mint-g++ \
      -DCMAKE_EXE_LINKER_FLAGS="-lncurses -ltinfo" \
      ..
```

Or if your MiNTOS toolchain has ncurses built without separate tinfo:
```bash
# For ncurses built with integrated terminfo
cmake -DCMAKE_SYSTEM_NAME=MiNT \
      -DCMAKE_C_COMPILER=m68k-atari-mint-gcc \
      -DCMAKE_CXX_COMPILER=m68k-atari-mint-g++ \
      -DCMAKE_EXE_LINKER_FLAGS="-lncurses" \
      ..
```

### Platform-Specific Notes

#### Linux
- Uses ncurses for terminal control
- UTF-8 locale support automatically detected
- Requires libncurses development headers

#### Windows
- Uses Windows Console API
- UTF-8 code page automatically enabled
- Supports Windows 10+ enhanced console features

#### Atari MiNTOS
- Compiled with m68k-atari-mint-gcc 4.6
- Uses C++98 standard with gnu++0x extensions when available
- No dynamic library dependencies
- Built-in syntax highlighting (no plugin system)

**Troubleshooting "error opening terminal":**

If you get "error opening terminal" on Atari Falcon, try these solutions:

1. **Set TERM environment variable:**
   ```bash
   export TERM=ansi
   ./subzero
   ```

2. **Alternative terminal types:**
   ```bash
   export TERM=vt100     # For VT100 compatibility
   export TERM=dumb      # For basic terminal support
   ```

3. **Check terminal info database:**
   - Ensure terminfo database is available in `/usr/share/terminfo/`
   - Or set `TERMINFO` to point to terminfo directory
   - Some MiNTOS installations may need terminfo files copied

4. **Console preparation:**
   - Make sure you're running from a proper console/terminal
   - Some Atari systems may need specific terminal setup

The program now includes automatic fallbacks for common terminal types (ansi, vt100, dumb) if the default fails.

## Usage

```bash
# Start with a new file
./subzero

# Open an existing file
./subzero filename.txt

# Open multiple files
./subzero file1.cpp file2.h

# Basic editing workflow
# 1. Use arrow keys or hjkl to navigate
# 2. Press 'i' to enter insert mode
# 3. Type your text
# 4. Press ESC to return to normal mode
# 5. Type ':w' to save, ':q' to quit
# 6. Use ':ls' to see all open buffers
# 7. Use ':b 2' to switch to buffer 2
```

## Architecture

### Core Components

- **Terminal Abstraction** (`terminal.h`, `terminal_factory.h`)
  - Cross-platform terminal interface
  - UTF-8 input/output handling
  - Platform-specific implementations (ncurses/Windows Console)

- **Buffer Management** (`buffer.h`)
  - UTF-8 aware text storage and manipulation
  - Cursor management with character-based positioning
  - File I/O operations
  - Multi-buffer support

- **Window System** (`window.h`)
  - Viewport management and scrolling
  - Line number display
  - Text rendering with UTF-8 support
  - Syntax highlighting integration
  - Coordinate conversion between buffer and screen

- **Editor Core** (`editor.h`)
  - Modal editing system (Normal, Insert, Visual, Command)
  - Multi-buffer management
  - Key binding management
  - Status bar and message display
  - Command processing and repeat counts
  - Main application loop

- **Syntax Highlighting** (`syntax_highlighter.h`, `syntax_highlighter_manager.h`)
  - Built-in C/C++ syntax highlighter
  - Extensible highlighting framework
  - File type detection based on extensions
  - Color-coded syntax elements

### File Structure
```
subzero/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── MANUAL.md               # User manual
├── include/                # Header files
│   ├── subzero.h          # Main header
│   ├── compat.h           # C++98 compatibility layer
│   ├── terminal*.h        # Terminal abstraction
│   ├── utf8_utils.h       # UTF-8 utilities
│   ├── buffer.h           # Text buffer
│   ├── window.h           # Display window
│   ├── editor.h           # Main editor
│   └── *syntax_highlighter*.h # Syntax highlighting
└── src/                   # Implementation files
    ├── main.cpp           # Application entry point
    ├── terminal*.cpp      # Platform-specific terminals
    ├── utf8_utils.cpp     # UTF-8 helper functions
    ├── buffer.cpp         # Buffer implementation
    ├── window.cpp         # Window implementation
    ├── editor.cpp         # Editor implementation
    └── *syntax_highlighter*.cpp # Syntax highlighting
```

## Roadmap

### Completed ✅
- [x] Cross-platform terminal abstraction (Linux, Windows, Atari)
- [x] UTF-8 text handling
- [x] Complete vi modes (Normal, Insert, Visual, Command)
- [x] File operations (open, save, new, reload)
- [x] Multi-buffer support with buffer management commands
- [x] Built-in syntax highlighting for C/C++
- [x] Core movement and editing commands with repeat counts
- [x] Status bar and user interface
- [x] Window system with scrolling
- [x] C++98 compatibility for embedded systems

### Planned 🚧
- [ ] Additional syntax highlighters (Python, JavaScript, etc.)
- [ ] Advanced search with regex support  
- [ ] Enhanced undo/redo system
- [ ] Configuration file support
- [ ] Mouse support
- [ ] Split windows
- [ ] Macro recording and playback

## Contributing

This project demonstrates clean, portable C++ design that works across a wide range of systems and compiler versions.

### Key Design Principles
- **UTF-8 first**: All text operations are UTF-8 aware
- **Cross-platform**: Clean abstraction over platform differences  
- **C++98 compatible**: Works with older compilers while leveraging modern design patterns
- **Vi compatibility**: Familiar key bindings and behavior
- **No external dependencies**: Self-contained with built-in functionality
- **Embedded-friendly**: Suitable for resource-constrained systems

### C++98 Compatibility Features
- **TR1 smart pointers**: Memory management without C++11 dependencies
- **Traditional iterators**: No range-based for loops
- **Manual memory management**: RAII principles with explicit cleanup
- **Compatible standard library**: Uses only C++98 standard library features
- **Cross-compiler support**: Works with GCC 4.6+ through modern compilers

## License

[License information to be added]

---

**Subzero** - A minimalist vi clone for modern development