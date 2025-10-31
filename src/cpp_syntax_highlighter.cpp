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
    std::vector<std::string> extensions;
    extensions.push_back("c");
    extensions.push_back("cpp");
    extensions.push_back("cxx");
    extensions.push_back("cc");
    extensions.push_back("c++");
    extensions.push_back("h");
    extensions.push_back("hpp");
    extensions.push_back("hxx");
    extensions.push_back("hh");
    extensions.push_back("h++");
    return extensions;
}

bool CppSyntaxHighlighter::canHighlight(const std::string& filename, const std::string& /*content_sample*/) const {
    // Check by extension
    std::vector<std::string> extensions = getSupportedExtensions();
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string ext = filename.substr(dot_pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        for (std::vector<std::string>::const_iterator it = extensions.begin(); it != extensions.end(); ++it) {
            if (ext == *it) {
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
            SyntaxToken token(pos, line_len - pos, Color::GREEN);
            result.tokens.push_back(token);
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
            SyntaxToken token(start, pos - start, Color::GREEN);
            result.tokens.push_back(token);
            continue;
        }
        
        // Preprocessor directives
        if (line[pos] == '#') {
            size_t start = pos;
            while (pos < line_len && !std::isspace(line[pos])) {
                pos++;
            }
            SyntaxToken token(start, pos - start, Color::MAGENTA);
            result.tokens.push_back(token);
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
            SyntaxToken token(start, pos - start, Color::YELLOW);
            result.tokens.push_back(token);
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
            SyntaxToken token(start, pos - start, Color::YELLOW);
            result.tokens.push_back(token);
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
            SyntaxToken token(start, pos - start, Color::CYAN);
            result.tokens.push_back(token);
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
                SyntaxToken token(start, pos - start, Color::BLUE);
                result.tokens.push_back(token);
            } else if (m_types.count(word)) {
                SyntaxToken token(start, pos - start, Color::BRIGHT_CYAN);
                result.tokens.push_back(token);
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
                    SyntaxToken token(start, 2, Color::RED);
                    result.tokens.push_back(token);
                    continue;
                }
            }
            
            // Single character operators
            char c = line[pos];
            if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
                c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
                c == '|' || c == '^' || c == '~' || c == '?' || c == ':') {
                SyntaxToken token(start, 1, Color::RED);
                result.tokens.push_back(token);
            }
            pos++;
            continue;
        }
        
        pos++;
    }
    
    return result;
}

void CppSyntaxHighlighter::initializeKeywords() {
    // C++ keywords
    m_keywords.insert("auto");
    m_keywords.insert("break");
    m_keywords.insert("case");
    m_keywords.insert("char");
    m_keywords.insert("const");
    m_keywords.insert("continue");
    m_keywords.insert("default");
    m_keywords.insert("do");
    m_keywords.insert("double");
    m_keywords.insert("else");
    m_keywords.insert("enum");
    m_keywords.insert("extern");
    m_keywords.insert("float");
    m_keywords.insert("for");
    m_keywords.insert("goto");
    m_keywords.insert("if");
    m_keywords.insert("int");
    m_keywords.insert("long");
    m_keywords.insert("register");
    m_keywords.insert("return");
    m_keywords.insert("short");
    m_keywords.insert("signed");
    m_keywords.insert("sizeof");
    m_keywords.insert("static");
    m_keywords.insert("struct");
    m_keywords.insert("switch");
    m_keywords.insert("typedef");
    m_keywords.insert("union");
    m_keywords.insert("unsigned");
    m_keywords.insert("void");
    m_keywords.insert("volatile");
    m_keywords.insert("while");
    
    // C++ specific keywords
    m_keywords.insert("class");
    m_keywords.insert("private");
    m_keywords.insert("protected");
    m_keywords.insert("public");
    m_keywords.insert("this");
    m_keywords.insert("new");
    m_keywords.insert("delete");
    m_keywords.insert("template");
    m_keywords.insert("typename");
    m_keywords.insert("namespace");
    m_keywords.insert("using");
    m_keywords.insert("try");
    m_keywords.insert("catch");
    m_keywords.insert("throw");
    m_keywords.insert("virtual");
    m_keywords.insert("explicit");
    m_keywords.insert("inline");
    m_keywords.insert("friend");
    m_keywords.insert("operator");
    
    // Built-in types
    m_types.insert("bool");
    m_types.insert("true");
    m_types.insert("false");
    m_types.insert("size_t");
    m_types.insert("ptrdiff_t");
    m_types.insert("wchar_t");
    m_types.insert("string");
    m_types.insert("vector");
    m_types.insert("map");
    m_types.insert("set");
    m_types.insert("list");
    m_types.insert("pair");
    m_types.insert("iterator");
    m_types.insert("const_iterator");
    m_types.insert("FILE");
}

} // namespace subzero