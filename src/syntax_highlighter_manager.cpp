#include "syntax_highlighter_manager.h"
#include "cpp_syntax_highlighter.h"
#include "markdown_syntax_highlighter.h"
#include <algorithm>

namespace subzero {

SyntaxHighlighterManager::SyntaxHighlighterManager() {
    registerBuiltinHighlighters();
    buildExtensionMap();
}

void SyntaxHighlighterManager::registerBuiltinHighlighters() {
    m_highlighters.push_back(new CppSyntaxHighlighter());
    m_highlighters.push_back(new MarkdownSyntaxHighlighter());
    
    // Add more highlighters here as needed
    // m_highlighters.push_back(new PythonSyntaxHighlighter());
    // m_highlighters.push_back(new JavaScriptSyntaxHighlighter());
}

SyntaxHighlighterManager::~SyntaxHighlighterManager() {
    for (std::vector<ISyntaxHighlighter*>::iterator it = m_highlighters.begin(); 
         it != m_highlighters.end(); ++it) {
        delete *it;
    }
}

void SyntaxHighlighterManager::buildExtensionMap() {
    for (std::vector<ISyntaxHighlighter*>::iterator highlighter_it = m_highlighters.begin(); 
         highlighter_it != m_highlighters.end(); ++highlighter_it) {
        std::vector<std::string> extensions = (*highlighter_it)->getSupportedExtensions();
        for (std::vector<std::string>::iterator ext_it = extensions.begin(); 
             ext_it != extensions.end(); ++ext_it) {
            // Convert extension to lowercase
            std::string lower_ext = *ext_it;
            std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
            m_extension_map[lower_ext] = *highlighter_it;
        }
    }
}

ISyntaxHighlighter* SyntaxHighlighterManager::getHighlighterForFile(const std::string& filename) const {
    if (filename.empty()) {
        return NULL;
    }
    
    // Extract file extension
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == filename.length() - 1) {
        return NULL;
    }
    
    std::string extension = filename.substr(dot_pos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Look up in extension map - this ensures only ONE highlighter per extension
    std::map<std::string, ISyntaxHighlighter*>::const_iterator it = m_extension_map.find(extension);
    if (it != m_extension_map.end()) {
        return it->second;
    }
    
    // No highlighter found for this extension
    return NULL;
}

} // namespace subzero