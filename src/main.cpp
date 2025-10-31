#include "subzero.h"
#include <iostream>

int main(int argc, char* argv[]) {
    using namespace subzero;
    
    // Create terminal
    std::unique_ptr<ITerminal> terminal = TerminalFactory::create();
    if (!terminal.get()) {
        std::cerr << "Failed to create terminal for platform: " 
                  << TerminalFactory::getPlatformName() << std::endl;
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