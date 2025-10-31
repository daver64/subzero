#pragma once
#include "syntax_highlighter.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#ifdef WINDOWS_PLATFORM
#include <windows.h>
typedef HMODULE PluginHandle;
#else
#include <dlfcn.h>
typedef void* PluginHandle;
#endif

namespace subzero {

struct LoadedPlugin {
    PluginHandle handle;
    std::unique_ptr<ISyntaxHighlighter> highlighter;
    CreateHighlighterFunc create_func;
    DestroyHighlighterFunc destroy_func;
    std::string filename;
    std::string info;
    
    LoadedPlugin() : handle(nullptr), create_func(nullptr), destroy_func(nullptr) {}
};

class PluginManager {
private:
    std::vector<std::unique_ptr<LoadedPlugin>> m_plugins;
    std::unordered_map<std::string, ISyntaxHighlighter*> m_extension_map;
    std::string m_plugin_directory;
    
public:
    PluginManager();
    ~PluginManager();
    
    // Plugin loading
    bool loadPlugin(const std::string& plugin_path);
    bool loadPluginsFromDirectory(const std::string& directory_path);
    void unloadAllPlugins();
    
    // Syntax highlighting
    ISyntaxHighlighter* getHighlighterForFile(const std::string& filename) const;
    ISyntaxHighlighter* getHighlighterByName(const std::string& name) const;
    
    // Plugin management
    std::vector<std::string> getLoadedPluginNames() const;
    size_t getPluginCount() const { return m_plugins.size(); }
    
    // Configuration
    void setPluginDirectory(const std::string& directory) { m_plugin_directory = directory; }
    const std::string& getPluginDirectory() const { return m_plugin_directory; }
    
private:
    bool loadPluginFromHandle(PluginHandle handle, const std::string& filename);
    void unloadPlugin(LoadedPlugin* plugin);
    std::string getFileExtension(const std::string& filename) const;
    void updateExtensionMap();
    
    // Platform-specific functions
    PluginHandle openLibrary(const std::string& path);
    void closeLibrary(PluginHandle handle);
    void* getSymbol(PluginHandle handle, const char* symbol_name);
    std::string getLastError();
};

} // namespace subzero