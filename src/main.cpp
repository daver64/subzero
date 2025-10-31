#include "subzero.h"
#include <iostream>
#include <cstdlib>  // For getenv

int main(int argc, char* argv[]) {
    using namespace subzero;
    
    // Show terminal environment info for debugging
    const char* term_env = getenv("TERM");
    const char* terminfo_dirs = getenv("TERMINFO_DIRS");
    const char* terminfo = getenv("TERMINFO");
    
    // Create terminal
    std::unique_ptr<ITerminal> terminal = TerminalFactory::create();
    if (!terminal.get()) {
        std::cerr << "Failed to create terminal for platform: " 
                  << TerminalFactory::getPlatformName() << std::endl;
        std::cerr << "Terminal debugging info:" << std::endl;
        std::cerr << "  TERM=" << (term_env ? term_env : "not set") << std::endl;
        std::cerr << "  TERMINFO=" << (terminfo ? terminfo : "not set") << std::endl;
        std::cerr << "  TERMINFO_DIRS=" << (terminfo_dirs ? terminfo_dirs : "not set") << std::endl;
        std::cerr << "Try setting TERM environment variable, e.g.: export TERM=ansi" << std::endl;
        return 1;
    }
    
    // Create editor (convert unique_ptr to shared_ptr)
    shared_ptr<ITerminal> shared_terminal(terminal.release());
    Editor editor(shared_terminal);
    
    // Open file if provided, otherwise create new file
    if (argc > 1) {
        if (!editor.openFile(argv[1])) {
            std::cerr << "Warning: Could not open file: " << argv[1] << std::endl;
            // Fall back to new file
            editor.newFile();
        }
    } else {
        // No file specified, create new file
        editor.newFile();
    }
    
    // Run editor
    editor.run();
    
    return 0;
}