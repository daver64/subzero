#pragma once
#include "syntax_highlighter.h"
#include <vector>
#include <memory>
#include <map>

namespace subzero {

class SyntaxHighlighterManager {
private:
    std::vector<std::unique_ptr<ISyntaxHighlighter>> m_highlighters;
    std::map<std::string, ISyntaxHighlighter*> m_extension_map;
    
    void registerBuiltinHighlighters();
    void buildExtensionMap();
    
public:
    SyntaxHighlighterManager();
    ~SyntaxHighlighterManager() = default;
    
    // Get highlighter for a specific file
    ISyntaxHighlighter* getHighlighterForFile(const std::string& filename) const;
    
    // Get all available highlighters
    const std::vector<std::unique_ptr<ISyntaxHighlighter>>& getHighlighters() const { return m_highlighters; }
    
    // Get highlighter count
    size_t getHighlighterCount() const { return m_highlighters.size(); }
};

} // namespace subzero