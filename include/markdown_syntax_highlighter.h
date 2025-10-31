#pragma once
#include "syntax_highlighter.h"
#include <string>
#include <vector>

namespace subzero {

class MarkdownSyntaxHighlighter : public ISyntaxHighlighter {
public:
    MarkdownSyntaxHighlighter();
    virtual ~MarkdownSyntaxHighlighter();
    
    // ISyntaxHighlighter interface
    virtual std::string getName() const;
    virtual std::string getVersion() const;
    virtual std::vector<std::string> getSupportedExtensions() const;
    virtual bool canHighlight(const std::string& filename, const std::string& content_sample) const;
    virtual SyntaxHighlightResult highlightLine(const std::string& line, size_t line_number, 
                                               const std::vector<std::string>& context_lines) const;

private:
    // Helper methods for different markdown elements
    void highlightHeaders(const std::string& line, SyntaxHighlightResult& result) const;
    void highlightEmphasis(const std::string& line, SyntaxHighlightResult& result) const;
    void highlightCode(const std::string& line, SyntaxHighlightResult& result) const;
    void highlightLinks(const std::string& line, SyntaxHighlightResult& result) const;
    void highlightLists(const std::string& line, SyntaxHighlightResult& result) const;
    void highlightBlockquotes(const std::string& line, SyntaxHighlightResult& result) const;
    
    // Utility methods
    bool isCodeBlock(const std::string& line) const;
    size_t findNextChar(const std::string& line, char ch, size_t start_pos) const;
    size_t findMatchingChar(const std::string& line, char open_char, char close_char, size_t start_pos) const;
};

} // namespace subzero