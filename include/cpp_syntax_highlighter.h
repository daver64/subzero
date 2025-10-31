#pragma once
#include "syntax_highlighter.h"
#include <set>

namespace subzero {

class CppSyntaxHighlighter : public ISyntaxHighlighter {
private:
    std::set<std::string> m_keywords;
    std::set<std::string> m_types;
    
    void initializeKeywords();
    
public:
    CppSyntaxHighlighter();
    
    std::string getName() const;
    std::string getVersion() const;
    std::vector<std::string> getSupportedExtensions() const;
    bool canHighlight(const std::string& filename, const std::string& content_sample) const;
    SyntaxHighlightResult highlightLine(const std::string& line, size_t line_number, 
                                       const std::vector<std::string>& context_lines) const;
};

} // namespace subzero