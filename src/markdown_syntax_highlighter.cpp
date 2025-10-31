#include "markdown_syntax_highlighter.h"
#include <algorithm>
#include <cctype>

namespace subzero {

MarkdownSyntaxHighlighter::MarkdownSyntaxHighlighter() {
}

MarkdownSyntaxHighlighter::~MarkdownSyntaxHighlighter() {
}

std::string MarkdownSyntaxHighlighter::getName() const {
    return "Markdown";
}

std::string MarkdownSyntaxHighlighter::getVersion() const {
    return "1.0.0";
}

std::vector<std::string> MarkdownSyntaxHighlighter::getSupportedExtensions() const {
    std::vector<std::string> extensions;
    extensions.push_back("md");
    extensions.push_back("markdown");
    extensions.push_back("mdown");
    extensions.push_back("mkd");
    extensions.push_back("mdx");
    return extensions;
}

bool MarkdownSyntaxHighlighter::canHighlight(const std::string& filename, const std::string& /* content_sample */) const {
    if (filename.empty()) {
        return false;
    }
    
    // Check if file has markdown extension
    std::vector<std::string> extensions = getSupportedExtensions();
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos && dot_pos < filename.length() - 1) {
        std::string ext = filename.substr(dot_pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        for (std::vector<std::string>::const_iterator it = extensions.begin(); 
             it != extensions.end(); ++it) {
            if (ext == *it) {
                return true;
            }
        }
    }
    
    return false;
}

SyntaxHighlightResult MarkdownSyntaxHighlighter::highlightLine(const std::string& line, size_t /* line_number */, 
                                                             const std::vector<std::string>& /* context_lines */) const {
    SyntaxHighlightResult result;
    result.processed_line = line;
    
    if (line.empty()) {
        return result;
    }
    
    // Apply highlighting in order of precedence
    highlightHeaders(line, result);
    highlightBlockquotes(line, result);
    highlightLists(line, result);
    highlightCode(line, result);
    highlightEmphasis(line, result);
    highlightLinks(line, result);
    
    return result;
}

void MarkdownSyntaxHighlighter::highlightHeaders(const std::string& line, SyntaxHighlightResult& result) const {
    if (line.empty() || line[0] != '#') {
        return;
    }
    
    // Count leading #'s
    size_t hash_count = 0;
    size_t pos = 0;
    while (pos < line.length() && line[pos] == '#') {
        hash_count++;
        pos++;
    }
    
    if (hash_count > 0 && hash_count <= 6) {
        // Header markers in magenta
        result.tokens.push_back(SyntaxToken(0, hash_count, Color::MAGENTA, Color::BLACK, true));
        
        // Skip spaces after #'s
        while (pos < line.length() && line[pos] == ' ') {
            pos++;
        }
        
        // Header text in cyan, bold
        if (pos < line.length()) {
            result.tokens.push_back(SyntaxToken(pos, line.length() - pos, Color::CYAN, Color::BLACK, true));
        }
    }
}

void MarkdownSyntaxHighlighter::highlightEmphasis(const std::string& line, SyntaxHighlightResult& result) const {
    size_t pos = 0;
    
    while (pos < line.length()) {
        // Look for bold (**text** or __text__)
        if (pos + 1 < line.length() && 
            ((line[pos] == '*' && line[pos + 1] == '*') || 
             (line[pos] == '_' && line[pos + 1] == '_'))) {
            
            char marker = line[pos];
            size_t start = pos;
            pos += 2; // Skip opening markers
            
            // Find closing markers
            while (pos + 1 < line.length()) {
                if (line[pos] == marker && line[pos + 1] == marker) {
                    // Found closing markers
                    pos += 2; // Skip closing markers
                    
                    // Highlight the entire bold section in yellow, bold
                    result.tokens.push_back(SyntaxToken(start, pos - start, Color::YELLOW, Color::BLACK, true));
                    break;
                }
                pos++;
            }
        }
        // Look for italic (*text* or _text_)
        else if (line[pos] == '*' || line[pos] == '_') {
            char marker = line[pos];
            size_t start = pos;
            pos++; // Skip opening marker
            
            // Find closing marker
            while (pos < line.length()) {
                if (line[pos] == marker) {
                    pos++; // Include closing marker
                    
                    // Highlight the entire italic section in bright yellow
                    result.tokens.push_back(SyntaxToken(start, pos - start, Color::BRIGHT_YELLOW, Color::BLACK, false, true));
                    break;
                }
                pos++;
            }
        } else {
            pos++;
        }
    }
}

void MarkdownSyntaxHighlighter::highlightCode(const std::string& line, SyntaxHighlightResult& result) const {
    size_t pos = 0;
    
    // Check for code blocks (```)
    if (line.length() >= 3 && line.substr(0, 3) == "```") {
        result.tokens.push_back(SyntaxToken(0, line.length(), Color::GREEN, Color::BLACK, false));
        return;
    }
    
    // Look for inline code (`text`)
    while (pos < line.length()) {
        if (line[pos] == '`') {
            size_t start = pos;
            pos++; // Skip opening backtick
            
            // Find closing backtick
            while (pos < line.length()) {
                if (line[pos] == '`') {
                    pos++; // Include closing backtick
                    
                    // Highlight the entire code section in green
                    result.tokens.push_back(SyntaxToken(start, pos - start, Color::GREEN, Color::BLACK, false));
                    break;
                }
                pos++;
            }
        } else {
            pos++;
        }
    }
}

void MarkdownSyntaxHighlighter::highlightLinks(const std::string& line, SyntaxHighlightResult& result) const {
    size_t pos = 0;
    
    while (pos < line.length()) {
        // Look for markdown links [text](url)
        if (line[pos] == '[') {
            size_t link_start = pos;
            pos++; // Skip opening bracket
            
            // Find closing bracket
            size_t bracket_end = findNextChar(line, ']', pos);
            if (bracket_end != std::string::npos && bracket_end + 1 < line.length() && line[bracket_end + 1] == '(') {
                // Find closing parenthesis
                size_t paren_end = findNextChar(line, ')', bracket_end + 2);
                if (paren_end != std::string::npos) {
                    // Highlight link text in blue
                    result.tokens.push_back(SyntaxToken(link_start, bracket_end - link_start + 1, Color::BLUE, Color::BLACK, false));
                    
                    // Highlight URL in bright blue
                    result.tokens.push_back(SyntaxToken(bracket_end + 1, paren_end - bracket_end, Color::BRIGHT_BLUE, Color::BLACK, false));
                    
                    pos = paren_end + 1;
                    continue;
                }
            }
        }
        // Look for bare URLs (http:// or https://)
        else if (pos + 7 < line.length() && 
                 (line.substr(pos, 7) == "http://" || line.substr(pos, 8) == "https://")) {
            size_t url_start = pos;
            
            // Find end of URL (space, newline, or end of line)
            while (pos < line.length() && line[pos] != ' ' && line[pos] != '\t' && line[pos] != '\n') {
                pos++;
            }
            
            // Highlight URL in bright blue
            result.tokens.push_back(SyntaxToken(url_start, pos - url_start, Color::BRIGHT_BLUE, Color::BLACK, false));
            continue;
        }
        
        pos++;
    }
}

void MarkdownSyntaxHighlighter::highlightLists(const std::string& line, SyntaxHighlightResult& result) const {
    if (line.empty()) {
        return;
    }
    
    // Skip leading whitespace
    size_t pos = 0;
    while (pos < line.length() && (line[pos] == ' ' || line[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= line.length()) {
        return;
    }
    
    // Check for unordered list markers (- * +)
    if (line[pos] == '-' || line[pos] == '*' || line[pos] == '+') {
        if (pos + 1 < line.length() && (line[pos + 1] == ' ' || line[pos + 1] == '\t')) {
            // Highlight list marker in red
            result.tokens.push_back(SyntaxToken(pos, 1, Color::RED, Color::BLACK, true));
        }
    }
    // Check for ordered list markers (1. 2. etc.)
    else if (std::isdigit(line[pos])) {
        size_t digit_start = pos;
        while (pos < line.length() && std::isdigit(line[pos])) {
            pos++;
        }
        
        if (pos < line.length() && line[pos] == '.' && 
            pos + 1 < line.length() && (line[pos + 1] == ' ' || line[pos + 1] == '\t')) {
            // Highlight number and dot in red
            result.tokens.push_back(SyntaxToken(digit_start, pos - digit_start + 1, Color::RED, Color::BLACK, true));
        }
    }
}

void MarkdownSyntaxHighlighter::highlightBlockquotes(const std::string& line, SyntaxHighlightResult& result) const {
    if (line.empty()) {
        return;
    }
    
    // Skip leading whitespace
    size_t pos = 0;
    while (pos < line.length() && (line[pos] == ' ' || line[pos] == '\t')) {
        pos++;
    }
    
    if (pos < line.length() && line[pos] == '>') {
        // Highlight blockquote marker in magenta
        result.tokens.push_back(SyntaxToken(pos, 1, Color::MAGENTA, Color::BLACK, false));
        
        // Skip space after >
        pos++;
        if (pos < line.length() && line[pos] == ' ') {
            pos++;
        }
        
        // Highlight blockquote content in bright cyan
        if (pos < line.length()) {
            result.tokens.push_back(SyntaxToken(pos, line.length() - pos, Color::BRIGHT_CYAN, Color::BLACK, false));
        }
    }
}

bool MarkdownSyntaxHighlighter::isCodeBlock(const std::string& line) const {
    return line.length() >= 3 && line.substr(0, 3) == "```";
}

size_t MarkdownSyntaxHighlighter::findNextChar(const std::string& line, char ch, size_t start_pos) const {
    for (size_t i = start_pos; i < line.length(); i++) {
        if (line[i] == ch) {
            return i;
        }
    }
    return std::string::npos;
}

size_t MarkdownSyntaxHighlighter::findMatchingChar(const std::string& line, char open_char, char close_char, size_t start_pos) const {
    int depth = 0;
    for (size_t i = start_pos; i < line.length(); i++) {
        if (line[i] == open_char) {
            depth++;
        } else if (line[i] == close_char) {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }
    return std::string::npos;
}

} // namespace subzero