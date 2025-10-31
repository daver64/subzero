# Subzero Text Editor Manual

## Overview
Subzero is a vi-like modal text editor with UTF-8 support, multi-buffer management, and syntax highlighting. This manual documents only the commands that are **actually implemented and working**.

## Modes

### Normal Mode
Default mode for navigation and commands. Press `ESC` from any mode to return to Normal mode.

### Insert Mode  
For text insertion. The status bar shows "INSERT" when active.

### Command Mode
For executing ex commands like save and quit. The status bar shows the command line starting with `:`.

### Search Mode
For entering search patterns. The status bar shows `/` or `?` followed by your search pattern.

### Visual Mode (Basic)
For text selection. Currently switches to visual mode but selection functionality is limited.

---

## Actually Implemented Commands

### Movement (Normal Mode)

| Command | Description |
|---------|-------------|
| `h` or `←` | Move cursor left |
| `j` or `↓` | Move cursor down |
| `k` or `↑` | Move cursor up |
| `l` or `→` | Move cursor right |
| `w` | Move to next word |
| `b` | Move to previous word |
| `0` | Move to beginning of line |
| `$` | Move to end of line |
| `gg` | Move to first line (proper vi command) |
| `G` | Move to last line |

### Text Editing (Normal Mode)

| Command | Description |
|---------|-------------|
| `i` | Enter Insert mode at cursor |
| `a` | Enter Insert mode after cursor |
| `o` | Insert new line below and enter Insert mode |
| `O` | Insert new line above and enter Insert mode |
| `x` | Delete character at cursor |
| `dd` | Delete entire line (proper vi command) |

### Copy/Paste (Normal Mode)

| Command | Description |
|---------|-------------|
| `yy` | Yank (copy) current line (proper vi command) |
| `p` | Paste after cursor |
| `P` | Paste before cursor |

### Undo (Normal Mode)

| Command | Description |
|---------|-------------|
| `u` | Undo last change |

### Mode Switching (Normal Mode)

| Command | Description |
|---------|-------------|
| `v` | Enter Visual mode (basic) |
| `V` | Enter Visual Line mode (basic) |
| `:` | Enter Command mode |
| `/` | Enter Search mode (forward) |
| `?` | Enter Search mode (backward) |

---

## Insert Mode Commands

| Command | Description |
|---------|-------------|
| `ESC` | Return to Normal mode |
| `Backspace` | Delete character before cursor |
| `Delete` | Delete character at cursor |
| `Tab` | Insert 4 spaces (configurable indentation) |
| `Enter` | Insert new line |
| `Arrow keys` | Navigate while staying in Insert mode |
| Any UTF-8 character | Insert the character |

---

## Command Mode (Ex Commands)

Enter Command mode with `:` from Normal mode.

### File Operations

| Command | Description |
|---------|-------------|
| `:q` | Quit (warns if file has unsaved changes) |
| `:q!` | Force quit (discards unsaved changes) |
| `:w` | Save current file |
| `:w filename` | Save as new filename |
| `:wq` | Save and quit |
| `:x` | Save and quit (same as `:wq`) |
| `:e filename` | Open/create file in new buffer |
| `:e!` | Reload current file (discard changes) |
| `:e! filename` | Force open file in new buffer |

### Buffer Management

| Command | Description |
|---------|-------------|
| `:ls` or `:buffers` | List all open buffers |
| `:b <number>` | Switch to buffer by number (e.g., `:b 2`) |
| `:bn` or `:bnext` | Switch to next buffer |
| `:bp` or `:bprev` | Switch to previous buffer |
| `:bd` or `:bdelete` | Close current buffer |
| `:bd!` or `:bdelete!` | Force close current buffer (discard changes) |

**Note**: In Command mode:
- `Backspace` - Delete last character from command (UTF-8 aware)
- `Enter` - Execute command
- `ESC` - Cancel and return to Normal mode

---

## Multi-Buffer System

Subzero supports working with multiple files simultaneously:

### Buffer Status Indicators
- **Current buffer**: Marked with `%` in buffer list
- **Modified buffer**: Marked with `+` in buffer list
- **Buffer numbers**: Start from 1 and increment automatically

### Opening Multiple Files
```bash
./subzero file1.cpp    # Open first file
:e file2.h             # Open second file in new buffer
:e main.cpp            # Open third file in new buffer
```

### Switching Between Buffers
- `:ls` - See all open buffers
- `:b 1` - Switch to buffer 1
- `:bn` - Next buffer (cycles through all)
- `:bp` - Previous buffer (cycles through all)

### Example Buffer List
```
Buffers:
  1   main.cpp
  2%+ header.h
  3   utils.cpp
```
- Buffer 2 is current (`%`) and modified (`+`)

---

## Search Mode

Enter Search mode with `/` (forward) or `?` (backward) from Normal mode.

| Command | Description |
|---------|-------------|
| `/` | Start forward search |
| `?` | Start backward search |

**Note**: Search entry works (you can type patterns) but actual searching is not yet implemented. The following are stubs:
- `n` - Next search result (shows "not implemented" message)
- `N` - Previous search result (shows "not implemented" message)

---

## Syntax Highlighting

Subzero includes a plugin-based syntax highlighting system.

### Supported Languages
- **C/C++**: Automatic detection for `.c`, `.cpp`, `.h`, `.hpp` files

### Color Scheme (C/C++)
- **Blue**: Keywords (`int`, `class`, `namespace`, `return`, etc.)
- **Bright Cyan**: Types (`bool`, `string`, `vector`, `true`, `false`)
- **Yellow**: Strings and character literals (`"hello"`, `'c'`)
- **Green**: Comments (`// single line`, `/* multi-line */`)
- **Magenta**: Preprocessor directives (`#include`, `#define`)
- **Cyan**: Numbers (`123`, `0xFF`, `3.14f`)
- **Red**: Operators (`+`, `==`, `->`, `++`)

### Automatic Detection
Syntax highlighting is automatically applied when opening files with recognized extensions.

---

## Status Bar Information

The status bar (bottom line) shows:
- **Current mode** (NORMAL, INSERT, COMMAND, SEARCH, VISUAL, VISUAL LINE)
- **Filename** or `[No Name]` for new files
- **Buffer number** (e.g., `[Buffer 2]`)
- **Modified indicator** `[+]` if file has unsaved changes
- **Cursor position** as `line:column`
- **Total line count**
- **Error/status messages**
- **Syntax highlighter name** when available

---

## File Operations

### Starting the Editor
```bash
./subzero              # Start with empty buffer
./subzero filename     # Open existing file in first buffer
```

### Creating New Files
- `:e newfile.txt` - Create new file in new buffer
- Type content in Insert mode
- `:w` - Save the file

### Working with Multiple Files
```bash
# Example workflow
./subzero main.cpp     # Open main file
:e header.h            # Open header in new buffer
:e utils.cpp           # Open utilities in new buffer
:ls                    # List all buffers
:b 1                   # Back to main.cpp
:w                     # Save current buffer
:bn                    # Next buffer
:wq                    # Save and quit (closes all buffers)
```

---

## UTF-8 Support

Subzero fully supports UTF-8 text:
- International characters display correctly
- Cursor movement is character-aware (not byte-aware)
- Proper text insertion and deletion for multi-byte characters
- UTF-8 aware backspace in command mode

---

## Repeat Counts

Many commands support repeat counts (prefix with number):

| Command | Description |
|---------|-------------|
| `3j` | Move down 3 lines |
| `5x` | Delete 5 characters |
| `2dd` | Delete 2 lines |
| `4yy` | Yank 4 lines |

---

## Known Limitations

### Not Yet Implemented
- Search functionality (beyond entering search mode)
- Redo (only undo works)
- Range commands in command mode
- Text objects (e.g., `dw`, `cw`)
- Proper visual selection operations
- Mouse support
- Configuration files
- Additional syntax highlighters

### Working As Expected
- Multi-buffer management ✓
- Basic syntax highlighting ✓
- UTF-8 support ✓
- Proper vi command sequences (`gg`, `dd`, `yy`) ✓
- Tab indentation ✓
- File operations ✓

---

## Tips

1. **Use multiple buffers** - `:e filename` to open additional files
2. **Check buffer status** - `:ls` shows all open files
3. **Save frequently** - `:w` to save current buffer
4. **Use proper vi commands** - `gg`, `dd`, `yy` work as expected
5. **Tab for indentation** - Inserts 4 spaces in Insert mode
6. **Syntax highlighting** - Automatic for C/C++ files
7. **Buffer navigation** - `:bn` and `:bp` for quick switching
8. **UTF-8 just works** - Type international characters normally

---

This manual reflects the current implementation state. Subzero is actively developed and new features are added regularly.