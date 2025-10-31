#include "cpp_syntax_highlighter.h"
#include <algorithm>

namespace subzero {

CppSyntaxHighlighter::CppSyntaxHighlighter() {
    initializeKeywords();
}

std::string CppSyntaxHighlighter::getName() const {
    return "C/C++ Syntax Highlighter";
}

std::string CppSyntaxHighlighter::getVersion() const {
    return "1.0.0";
}

std::vector<std::string> CppSyntaxHighlighter::getSupportedExtensions() const {
    return {"c", "cpp", "cxx", "cc", "c++", "h", "hpp", "hxx", "hh", "h++"};
}

bool CppSyntaxHighlighter::canHighlight(const std::string& filename, const std::string& /*content_sample*/) const {
    // Check by extension
    auto extensions = getSupportedExtensions();
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string ext = filename.substr(dot_pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        for (const auto& supported_ext : extensions) {
            if (ext == supported_ext) {
                return true;
            }
        }
    }
    
    return false;
}

SyntaxHighlightResult CppSyntaxHighlighter::highlightLine(const std::string& line, size_t /*line_number*/, 
                                   const std::vector<std::string>& /*context_lines*/) const {
    SyntaxHighlightResult result;
    result.processed_line = line;
    
    if (line.empty()) {
        return result;
    }
    
    size_t pos = 0;
    size_t line_len = line.length();
    
    while (pos < line_len) {
        // Skip whitespace
        if (std::isspace(line[pos])) {
            pos++;
            continue;
        }
        
        // Single line comments
        if (pos + 1 < line_len && line.substr(pos, 2) == "//") {
            result.tokens.emplace_back(pos, line_len - pos, Color::GREEN);
            break;
        }
        
        // Multiline comments
        if (pos + 1 < line_len && line.substr(pos, 2) == "/*") {
            size_t start = pos;
            pos += 2;
            while (pos + 1 < line_len) {
                if (line.substr(pos, 2) == "*/") {
                    pos += 2;
                    break;
                }
                pos++;
            }
            result.tokens.emplace_back(start, pos - start, Color::GREEN);
            continue;
        }
        
        // Preprocessor directives
        if (line[pos] == '#') {
            size_t start = pos;
            while (pos < line_len && !std::isspace(line[pos])) {
                pos++;
            }
            result.tokens.emplace_back(start, pos - start, Color::MAGENTA);
            continue;
        }
        
        // String literals
        if (line[pos] == '"') {
            size_t start = pos;
            pos++; // Skip opening quote
            while (pos < line_len) {
                if (line[pos] == '\\' && pos + 1 < line_len) {
                    pos += 2; // Skip escaped character
                } else if (line[pos] == '"') {
                    pos++; // Include closing quote
                    break;
                } else {
                    pos++;
                }
            }
            result.tokens.emplace_back(start, pos - start, Color::YELLOW);
            continue;
        }
        
        // Character literals
        if (line[pos] == '\'') {
            size_t start = pos;
            pos++; // Skip opening quote
            if (pos < line_len && line[pos] == '\\' && pos + 1 < line_len) {
                pos += 2; // Escaped character
            } else if (pos < line_len) {
                pos++; // Regular character
            }
            if (pos < line_len && line[pos] == '\'') {
                pos++; // Include closing quote
            }
            result.tokens.emplace_back(start, pos - start, Color::YELLOW);
            continue;
        }
        
        // Numbers
        if (std::isdigit(line[pos])) {
            size_t start = pos;
            while (pos < line_len && (std::isdigit(line[pos]) || line[pos] == '.' || 
                                     line[pos] == 'x' || line[pos] == 'X' || 
                                     std::isxdigit(line[pos]) || line[pos] == 'f' || 
                                     line[pos] == 'l' || line[pos] == 'u' || line[pos] == 'L' ||
                                     line[pos] == 'U')) {
                pos++;
            }
            result.tokens.emplace_back(start, pos - start, Color::CYAN);
            continue;
        }
        
        // Identifiers and keywords
        if (std::isalpha(line[pos]) || line[pos] == '_') {
            size_t start = pos;
            while (pos < line_len && (std::isalnum(line[pos]) || line[pos] == '_')) {
                pos++;
            }
            
            std::string word = line.substr(start, pos - start);
            
            if (m_keywords.count(word)) {
                result.tokens.emplace_back(start, pos - start, Color::BLUE);
            } else if (m_types.count(word)) {
                result.tokens.emplace_back(start, pos - start, Color::BRIGHT_CYAN);
            }
            continue;
        }
        
        // Operators
        if (std::ispunct(line[pos])) {
            size_t start = pos;
            // Check for multi-character operators
            if (pos + 1 < line_len) {
                std::string two_char = line.substr(pos, 2);
                if (two_char == "++" || two_char == "--" || two_char == "==" || 
                    two_char == "!=" || two_char == "<=" || two_char == ">=" ||
                    two_char == "&&" || two_char == "||" || two_char == "<<" ||
                    two_char == ">>" || two_char == "+=" || two_char == "-=" ||
                    two_char == "*=" || two_char == "/=" || two_char == "%=" ||
                    two_char == "::" || two_char == "->") {
                    pos += 2;
                    result.tokens.emplace_back(start, 2, Color::RED);
                    continue;
                }
            }
            
            // Single character operators
            char c = line[pos];
            if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
                c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
                c == '|' || c == '^' || c == '~' || c == '?' || c == ':') {
                result.tokens.emplace_back(start, 1, Color::RED);
            }
            pos++;
            continue;
        }
        
        pos++;
    }
    
    return result;
}

void CppSyntaxHighlighter::initializeKeywords() {
    // C keywords
    m_keywords = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "int", "long", "register", "return", "short", "signed", "sizeof", "static",
        "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
    };
    
    // C++ additional keywords
    m_keywords.insert({
        "class", "namespace", "template", "typename", "public", "private", "protected",
        "virtual", "override", "final", "explicit", "inline", "friend", "operator",
        "new", "delete", "this", "try", "catch", "throw", "using", "constexpr",
        "decltype", "nullptr", "static_assert", "thread_local", "alignas", "alignof",
        "noexcept", "consteval", "constinit", "concept", "requires"
    });
    
    // Common types
    m_types = {
        "bool", "true", "false", "nullptr_t", "size_t", "ptrdiff_t", "wchar_t",
        "char8_t", "char16_t", "char32_t", "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t",
        "string", "vector", "map", "set", "list", "array", "unique_ptr", "shared_ptr"
    };
}

} // namespace subzero