#include "subzero.h"
#include <iostream>
#include <cstdlib>  // For getenv
#include <cstdio>   // For printf

// Debug output control - disable debug output on MiNTOS by default
#ifdef MINTOS_PLATFORM
    // On Atari, minimize debug output to avoid screen clutter
    #define DEBUG_PRINT(fmt, ...) do {} while(0)
#else
    // On other platforms, allow debug output
    #define DEBUG_PRINT(fmt, ...) do { printf(fmt, ##__VA_ARGS__); fflush(stdout); } while(0)
#endif

int main(int argc, char* argv[]) {
    using namespace subzero;
    
    // Early diagnostic output to ensure we reach this point
    DEBUG_PRINT("SubZero starting...\n");
    DEBUG_PRINT("Platform: %s\n", TerminalFactory::getPlatformName().c_str());
    
    try {
        // Show terminal environment info for debugging
        const char* term_env = getenv("TERM");
        const char* terminfo_dirs = getenv("TERMINFO_DIRS");
        const char* terminfo = getenv("TERMINFO");
        
        DEBUG_PRINT("Environment check:\n");
        DEBUG_PRINT("  TERM=%s\n", term_env ? term_env : "not set");
        DEBUG_PRINT("  TERMINFO=%s\n", terminfo ? terminfo : "not set");
        DEBUG_PRINT("  TERMINFO_DIRS=%s\n", terminfo_dirs ? terminfo_dirs : "not set");
        
        // Create terminal
        DEBUG_PRINT("Creating terminal...\n");
        
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
        
        DEBUG_PRINT("Terminal created successfully\n");
        
        // Create editor (convert unique_ptr to shared_ptr)
        DEBUG_PRINT("Creating editor...\n");
        
        shared_ptr<ITerminal> shared_terminal(terminal.release());
        Editor editor(shared_terminal);
        
        DEBUG_PRINT("Editor created successfully\n");
        
        // Open file if provided, otherwise create new file
        if (argc > 1) {
            DEBUG_PRINT("Opening file: %s\n", argv[1]);
            if (!editor.openFile(argv[1])) {
                DEBUG_PRINT("Warning: Could not open file: %s\n", argv[1]);
                DEBUG_PRINT("Creating new file instead\n");
                // Fall back to new file
                editor.newFile();
            }
        } else {
            // No file specified, create new file
            DEBUG_PRINT("Creating new file\n");
            editor.newFile();
        }
        
        DEBUG_PRINT("Starting editor...\n");
        
        // Run editor
        editor.run();
        
        DEBUG_PRINT("Editor exited normally\n");
        
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