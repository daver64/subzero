#include "subzero.h"
#include <iostream>
#include <cstdlib>  // For getenv
#include <cstdio>   // For printf

int main(int argc, char* argv[]) {
    using namespace subzero;
    
    // Early diagnostic output to ensure we reach this point
    printf("SubZero starting...\n");
    printf("Platform: %s\n", TerminalFactory::getPlatformName().c_str());
    fflush(stdout);
    
    try {
        // Show terminal environment info for debugging
        const char* term_env = getenv("TERM");
        const char* terminfo_dirs = getenv("TERMINFO_DIRS");
        const char* terminfo = getenv("TERMINFO");
        
        printf("Environment check:\n");
        printf("  TERM=%s\n", term_env ? term_env : "not set");
        printf("  TERMINFO=%s\n", terminfo ? terminfo : "not set");
        printf("  TERMINFO_DIRS=%s\n", terminfo_dirs ? terminfo_dirs : "not set");
        fflush(stdout);
        
        // Create terminal
        printf("Creating terminal...\n");
        fflush(stdout);
        
        std::unique_ptr<ITerminal> terminal = TerminalFactory::create();
        if (!terminal.get()) {
            printf("ERROR: Failed to create terminal for platform: %s\n", 
                   TerminalFactory::getPlatformName().c_str());
            printf("Terminal debugging info:\n");
            printf("  TERM=%s\n", term_env ? term_env : "not set");
            printf("  TERMINFO=%s\n", terminfo ? terminfo : "not set");
            printf("  TERMINFO_DIRS=%s\n", terminfo_dirs ? terminfo_dirs : "not set");
            printf("Try setting TERM environment variable, e.g.: export TERM=ansi\n");
            fflush(stdout);
            return 1;
        }
        
        printf("Terminal created successfully\n");
        fflush(stdout);
        
        // Create editor (convert unique_ptr to shared_ptr)
        printf("Creating editor...\n");
        fflush(stdout);
        
        shared_ptr<ITerminal> shared_terminal(terminal.release());
        Editor editor(shared_terminal);
        
        printf("Editor created successfully\n");
        fflush(stdout);
        
        // Open file if provided, otherwise create new file
        if (argc > 1) {
            printf("Opening file: %s\n", argv[1]);
            fflush(stdout);
            if (!editor.openFile(argv[1])) {
                printf("Warning: Could not open file: %s\n", argv[1]);
                printf("Creating new file instead\n");
                fflush(stdout);
                // Fall back to new file
                editor.newFile();
            }
        } else {
            // No file specified, create new file
            printf("Creating new file\n");
            fflush(stdout);
            editor.newFile();
        }
        
        printf("Starting editor...\n");
        fflush(stdout);
        
        // Run editor
        editor.run();
        
        printf("Editor exited normally\n");
        fflush(stdout);
        
    } catch (const std::exception& e) {
        printf("EXCEPTION: %s\n", e.what());
        fflush(stdout);
        return 1;
    } catch (...) {
        printf("UNKNOWN EXCEPTION occurred\n");
        fflush(stdout);
        return 1;
    }
    
    return 0;
}