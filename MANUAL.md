# Subzero Text Editor Manual

## Overview
Subzero is a vi-like modal text editor with UTF-8 support, multi-buffer management, and built-in syntax highlighting. This manual documents only the commands that are **actually implemented and working**.

**Key Features:**
- **C++98 Compatible**: Works on modern systems and embedded platforms like Atari MiNTOS
- **Cross-platform**: Linux (ncurses), Windows (Console API), and embedded systems  
- **Built-in syntax highlighting**: No external dependencies required
- **Multi-buffer support**: Work with multiple files simultaneously
- **UTF-8 native**: Full international text support
- **Comprehensive help system**: Built-in `:help` command with complete documentation
- **Search functionality**: Pattern-based searching with navigation
- **Performance optimized**: Fast rendering and minimal screen flicker

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

### Movement (Normal Mode) with Repeat Counts

| Command | Description |
|---------|-------------|
| `h` or `←` | Move cursor left (supports repeat: `5h`) |
| `j` or `↓` | Move cursor down (supports repeat: `10j`) |
| `k` or `↑` | Move cursor up (supports repeat: `3k`) |
| `l` or `→` | Move cursor right (supports repeat: `2l`) |
| `w` | Move to next word |
| `b` | Move to previous word |
| `0` | Move to beginning of line |
| `$` | Move to end of line |
| `gg` | Move to first line (proper vi command sequence) |
| `G` | Move to last line |

### Text Editing (Normal Mode)

| Command | Description |
|---------|-------------|
| `i` | Enter Insert mode at cursor |
| `a` | Enter Insert mode after cursor |
| `o` | Insert new line below and enter Insert mode |
| `O` | Insert new line above and enter Insert mode |
| `x` | Delete character at cursor |
| `dd` | Delete entire line (supports repeat count) |

### Copy/Paste (Normal Mode)

| Command | Description |
|---------|-------------|
| `yy` | Yank (copy) current line (supports repeat count) |
| `p` | Paste after cursor (supports repeat count) |
| `P` | Paste before cursor (supports repeat count) |

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

### Help System

| Command | Description |
|---------|-------------|
| `:help` or `:h` | Open comprehensive help documentation in new buffer `*help*` |

**Help Features:**
- **Complete documentation**: All commands, modes, and features covered
- **Separate buffer**: Opens in `*help*` buffer, doesn't overwrite your files
- **Searchable content**: Use normal vi navigation to find information
- **Easy exit**: Use `:q` to close help and return to your work
- **Always available**: Built-in documentation, no external files needed

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

### Search Commands

| Command | Description |
|---------|-------------|
| `/pattern` | Search forward for pattern |
| `?pattern` | Search backward for pattern |
| `n` | Go to next search result |
| `N` | Go to previous search result |

### Search Features
- **String-based matching**: Searches for exact text patterns (no regex for C++98 compatibility)
- **UTF-8 aware**: Properly handles international characters in search patterns
- **Case-sensitive**: Exact character matching
- **Wrap-around**: Search continues from beginning/end of file when reaching end/beginning
- **Status feedback**: Shows search progress and "not found" messages
- **Pattern display**: Current search pattern shown in status bar

### Search Navigation
In Search mode:
- **Type pattern**: Enter your search text
- **Backspace**: Delete last character from pattern (UTF-8 aware)
- **Enter**: Execute search and return to Normal mode
- **ESC**: Cancel search and return to Normal mode

After searching (in Normal mode):
- **`n`**: Jump to next occurrence of last search pattern
- **`N`**: Jump to previous occurrence of last search pattern

---

## Built-in Syntax Highlighting

Subzero includes built-in syntax highlighting with no external dependencies.

### Supported Languages
- **C/C++**: Automatic detection for `.c`, `.cpp`, `.cxx`, `.cc`, `.c++`, `.h`, `.hpp`, `.hxx`, `.hh`, `.h++`
- **Markdown**: Automatic detection for `.md`, `.markdown`, `.mdown`, `.mkd`, `.mdx`

### Color Scheme (C/C++)
- **Blue**: Keywords (`int`, `class`, `namespace`, `return`, `if`, `for`, `while`, etc.)
- **Bright Cyan**: Types (`bool`, `string`, `vector`, `true`, `false`, `size_t`)
- **Yellow**: Strings and character literals (`"hello"`, `'c'`)
- **Green**: Comments (`// single line`, `/* multi-line */`)
- **Magenta**: Preprocessor directives (`#include`, `#define`)
- **Cyan**: Numbers (`123`, `0xFF`, `3.14f`, `0x1A2B`)
- **Red**: Operators (`+`, `==`, `->`, `++`, `&&`, `||`)

### Color Scheme (Markdown)
- **Cyan (Bold)**: Headers (`# Title`, `## Subtitle`)
- **Magenta (Bold)**: Header markers (`#`, `##`) and blockquote markers (`>`)
- **Yellow (Bold)**: Bold text (`**bold**`, `__bold__`)
- **Bright Yellow (Italic)**: Italic text (`*italic*`, `_italic_`)
- **Green**: Code blocks (` ```code``` `) and inline code (`` `code` ``)
- **Blue**: Link text (`[text]`)
- **Bright Blue**: URLs (`(url)`, `http://`, `https://`)
- **Red (Bold)**: List markers (`-`, `*`, `+`, `1.`, `2.`)
- **Bright Cyan**: Blockquote content

### Automatic Detection
- Syntax highlighting is automatically applied when opening files with recognized extensions
- File type detection is case-insensitive
- The active highlighter name is shown in the status bar when available

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
- Advanced search with regex support (uses string matching for C++98 compatibility)
- Redo functionality (only undo works)
- Range commands in command mode (e.g., `:1,5d`)
- Text objects (e.g., `dw`, `cw`, `diw`)
- Proper visual selection operations beyond mode switching
- Mouse support
- Configuration files
- Additional syntax highlighters beyond C/C++ and Markdown
- Macro recording and playback
- Split windows

### Working As Expected ✅
- **Multi-buffer management**: Complete buffer system with switching, listing, and management
- **Syntax highlighting**: C/C++ and Markdown with automatic detection
- **UTF-8 support**: Full international character support throughout
- **Vi command sequences**: Proper `gg`, `dd`, `yy` implementations
- **Tab indentation**: 4-space insertion in Insert mode
- **File operations**: All basic file I/O operations
- **Help system**: Comprehensive built-in documentation
- **Search functionality**: Pattern entry, execution, and navigation
- **Screen rendering**: Proper display updates and deletion handling
- **Performance**: Optimized for smooth operation on all platforms

### Recently Fixed ✅
- **Screen clearing**: Deletion artifacts properly removed
- **Buffer switching**: Clean transitions between files
- **Help buffer isolation**: Help doesn't overwrite file content
- **Search pattern handling**: UTF-8 aware pattern entry and matching

---

## Tips

1. **Use multiple buffers** - `:e filename` to open additional files, `:ls` to see all open files
2. **Get help anytime** - `:help` opens comprehensive documentation in a separate buffer
3. **Search efficiently** - Use `/pattern` to find text, then `n` and `N` to navigate results
4. **Save frequently** - `:w` to save current buffer, `:wq` to save and quit
5. **Use proper vi commands** - `gg`, `dd`, `yy` work as expected with repeat counts
6. **Tab for indentation** - Inserts 4 spaces in Insert mode
7. **Syntax highlighting** - Automatic for C/C++ and Markdown files
8. **Buffer navigation** - `:bn` and `:bp` for quick switching between files
9. **UTF-8 just works** - Type international characters normally, everything is character-aware
10. **Performance optimized** - Fast rendering and minimal screen updates for smooth editing
11. **Clean deletions** - Text deletion properly clears screen areas (no ghost characters)
12. **Help is searchable** - Use normal vi navigation in help buffer to find specific topics

---

This manual reflects the current implementation state. Subzero is actively developed and new features are added regularly.