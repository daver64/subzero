#pragma once
#include "terminal_types.h"
#include <string>
#include <vector>
#include <memory>

namespace subzero {

struct SyntaxToken {
    size_t start_pos;      // Character position in line (not byte position)
    size_t length;         // Length in characters
    Color::Value color;    // Foreground color
    Color::Value bg_color; // Background color (usually BLACK)
    bool bold;             // Bold text
    bool italic;           // Italic text (if supported)
    
    SyntaxToken(size_t start, size_t len, Color::Value fg = Color::WHITE, 
                Color::Value bg = Color::BLACK, bool b = false, bool i = false)
        : start_pos(start), length(len), color(fg), bg_color(bg), bold(b), italic(i) {}
};

struct SyntaxHighlightResult {
    std::vector<SyntaxToken> tokens;
    std::string processed_line;  // Line with any processing applied
    
    void clear() {
        tokens.clear();
        processed_line.clear();
    }
};

// Plugin interface that syntax highlighters must implement
class ISyntaxHighlighter {
public:
    virtual ~ISyntaxHighlighter() = default;
    
    // Plugin metadata
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    
    // Syntax highlighting
    virtual bool canHighlight(const std::string& filename, const std::string& content_sample) const = 0;
    virtual SyntaxHighlightResult highlightLine(const std::string& line, size_t line_number, 
                                               const std::vector<std::string>& context_lines) const = 0;
    
    // Configuration
    virtual void setColorScheme(const std::string& /*scheme_name*/) {}
    virtual void setOption(const std::string& /*key*/, const std::string& /*value*/) {}
};

// C interface for plugins (exported from DLL/SO)
extern "C" {
    // Plugin factory function - creates highlighter instance
    typedef ISyntaxHighlighter* (*CreateHighlighterFunc)();
    
    // Plugin cleanup function - destroys highlighter instance
    typedef void (*DestroyHighlighterFunc)(ISyntaxHighlighter* highlighter);
    
    // Plugin info function - returns basic plugin information
    typedef const char* (*GetPluginInfoFunc)();
}

// Plugin function names (must be exported by plugins)
#define CREATE_HIGHLIGHTER_FUNC_NAME "createHighlighter"
#define DESTROY_HIGHLIGHTER_FUNC_NAME "destroyHighlighter"
#define GET_PLUGIN_INFO_FUNC_NAME "getPluginInfo"

} // namespace subzero