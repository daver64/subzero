#pragma once
#include "syntax_highlighter.h"
#include <unordered_set>

namespace subzero {

class CppSyntaxHighlighter : public ISyntaxHighlighter {
private:
    std::unordered_set<std::string> m_keywords;
    std::unordered_set<std::string> m_types;
    
    void initializeKeywords();
    
public:
    CppSyntaxHighlighter();
    
    std::string getName() const override;
    std::string getVersion() const override;
    std::vector<std::string> getSupportedExtensions() const override;
    bool canHighlight(const std::string& filename, const std::string& content_sample) const override;
    SyntaxHighlightResult highlightLine(const std::string& line, size_t line_number, 
                                       const std::vector<std::string>& context_lines) const override;
};

} // namespace subzero