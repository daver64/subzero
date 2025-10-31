#include "syntax_highlighter_manager.h"
#include "cpp_syntax_highlighter.h"
#include <algorithm>

namespace subzero {

SyntaxHighlighterManager::SyntaxHighlighterManager() {
    registerBuiltinHighlighters();
    buildExtensionMap();
}

void SyntaxHighlighterManager::registerBuiltinHighlighters() {
    // Register C++ syntax highlighter
    m_highlighters.push_back(std::unique_ptr<ISyntaxHighlighter>(new CppSyntaxHighlighter()));
    
    // Future highlighters can be added here:
    // m_highlighters.push_back(std::unique_ptr<ISyntaxHighlighter>(new PythonSyntaxHighlighter()));
    // m_highlighters.push_back(std::unique_ptr<ISyntaxHighlighter>(new JavaScriptSyntaxHighlighter()));
}

void SyntaxHighlighterManager::buildExtensionMap() {
    m_extension_map.clear();
    
    for (const auto& highlighter : m_highlighters) {
        auto extensions = highlighter->getSupportedExtensions();
        for (const auto& ext : extensions) {
            // Convert to lowercase for case-insensitive matching
            std::string lower_ext = ext;
            std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
            m_extension_map[lower_ext] = highlighter.get();
        }
    }
}

ISyntaxHighlighter* SyntaxHighlighterManager::getHighlighterForFile(const std::string& filename) const {
    if (filename.empty()) {
        return nullptr;
    }
    
    // Extract file extension
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == filename.length() - 1) {
        return nullptr;
    }
    
    std::string extension = filename.substr(dot_pos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Look up in extension map
    auto it = m_extension_map.find(extension);
    if (it != m_extension_map.end()) {
        return it->second;
    }
    
    // If no direct extension match, try asking each highlighter
    // (for content-based detection)
    for (const auto& highlighter : m_highlighters) {
        if (highlighter->canHighlight(filename, "")) {
            return highlighter.get();
        }
    }
    
    return nullptr;
}

} // namespace subzero