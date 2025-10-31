# Subzero Text Editor

A lightweight, cross-platform vi-like text editor written in modern C++ with full UTF-8 support.

## Features

### Core Functionality
- **Cross-platform**: Runs on Linux (ncurses) and Windows (Console API)
- **UTF-8 native**: Full international text support with proper character-aware operations
- **Vi-compatible**: Familiar vi key bindings and modal editing
- **Modern C++**: Clean architecture using C++17, smart pointers, and RAII

### Vi Modes Implemented
- **Normal Mode**: Movement, editing commands, and navigation
- **Insert Mode**: Text insertion with UTF-8 support
- **Visual Mode**: Text selection (basic implementation)
- **Command Mode**: Ex commands (`:w`, `:q`, `:wq`, etc.)
- **Search Mode**: Forward and backward search (framework)

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
| `dd` | Delete line |
| `yy` | Yank (copy) line |
| `p`, `P` | Paste after/before cursor |
| `u` | Undo |
| `v`, `V` | Enter visual/visual line mode |
| `:` | Enter command mode |
| `/`, `?` | Search forward/backward |
| `n`, `N` | Next/previous search result |
| `q` | Quit (simplified) |

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

### Display Features
- **Line numbers**: Configurable line number display
- **Status bar**: Shows current mode, filename, cursor position, and line count
- **UTF-8 rendering**: Proper display of international characters
- **Tab expansion**: Configurable tab width
- **Viewport management**: Smooth scrolling and cursor tracking

## Building

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- **Linux**: ncurses library (`sudo apt install libncurses5-dev` on Ubuntu)
- **Windows**: Windows SDK (Console API)

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

### Platform-Specific Notes

#### Linux
- Uses ncurses for terminal control
- UTF-8 locale support automatically detected
- Requires libncurses development headers

#### Windows
- Uses Windows Console API
- UTF-8 code page automatically enabled
- Supports Windows 10+ enhanced console features

## Usage

```bash
# Start with a new file
./subzero

# Open an existing file
./subzero filename.txt

# Basic editing workflow
# 1. Use arrow keys or hjkl to navigate
# 2. Press 'i' to enter insert mode
# 3. Type your text
# 4. Press ESC to return to normal mode
# 5. Type ':w' to save, ':q' to quit
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
  - Undo/redo framework

- **Window System** (`window.h`)
  - Viewport management and scrolling
  - Line number display
  - Text rendering with UTF-8 support
  - Coordinate conversion between buffer and screen

- **Editor Core** (`editor.h`)
  - Modal editing system (Normal, Insert, Visual, Command)
  - Key binding management
  - Status bar and message display
  - Main application loop

### File Structure
```
subzero/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── include/                # Header files
│   ├── subzero.h          # Main header
│   ├── terminal*.h        # Terminal abstraction
│   ├── utf8_utils.h       # UTF-8 utilities
│   ├── buffer.h           # Text buffer
│   ├── window.h           # Display window
│   └── editor.h           # Main editor
└── src/                   # Implementation files
    ├── main.cpp           # Application entry point
    ├── terminal*.cpp      # Platform-specific terminals
    ├── utf8_utils.cpp     # UTF-8 helper functions
    ├── buffer.cpp         # Buffer implementation
    ├── window.cpp         # Window implementation
    └── editor.cpp         # Editor implementation
```

## Roadmap

### Completed ✅
- [x] Cross-platform terminal abstraction
- [x] UTF-8 text handling
- [x] Basic vi modes (Normal, Insert, Command)
- [x] File operations (open, save, new)
- [x] Core movement and editing commands
- [x] Status bar and user interface
- [x] Window system with scrolling

### Planned 🚧
- [ ] Plugin system for syntax highlighting
- [ ] Advanced search with regex support
- [ ] Multiple buffers/tabs
- [ ] More complete vi command set
- [ ] Configuration file support
- [ ] Enhanced undo/redo system
- [ ] Mouse support
- [ ] Split windows

## Contributing

This is a learning project focused on building a clean, modern vi clone. The architecture is designed to be extensible and maintainable.

### Key Design Principles
- **UTF-8 first**: All text operations are UTF-8 aware
- **Cross-platform**: Clean abstraction over platform differences
- **Modern C++**: Leveraging C++17 features and best practices
- **Vi compatibility**: Familiar key bindings and behavior
- **Extensible**: Plugin-ready architecture for future enhancements

## License

[License information to be added]

---

**Subzero** - A minimalist vi clone for modern development