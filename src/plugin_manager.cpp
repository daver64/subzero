#include "plugin_manager.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace subzero {

PluginManager::PluginManager() {
    // Set default plugin directory
#ifdef WINDOWS_PLATFORM
    m_plugin_directory = "./plugins";
#else
    m_plugin_directory = "./plugins";
#endif
}

PluginManager::~PluginManager() {
    unloadAllPlugins();
}

bool PluginManager::loadPlugin(const std::string& plugin_path) {
    PluginHandle handle = openLibrary(plugin_path);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << plugin_path << " - " << getLastError() << std::endl;
        return false;
    }
    
    return loadPluginFromHandle(handle, plugin_path);
}

bool PluginManager::loadPluginsFromDirectory(const std::string& directory_path) {
    try {
        if (!std::filesystem::exists(directory_path)) {
            std::cerr << "Plugin directory does not exist: " << directory_path << std::endl;
            return false;
        }
        
        bool loaded_any = false;
        
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();
                
#ifdef WINDOWS_PLATFORM
                if (extension == ".dll") {
#else
                if (extension == ".so") {
#endif
                    std::cout << "Loading plugin: " << filename << std::endl;
                    if (loadPlugin(entry.path().string())) {
                        loaded_any = true;
                        std::cout << "Successfully loaded: " << filename << std::endl;
                    } else {
                        std::cerr << "Failed to load: " << filename << std::endl;
                    }
                }
            }
        }
        
        updateExtensionMap();
        return loaded_any;
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error reading plugin directory: " << e.what() << std::endl;
        return false;
    }
}

void PluginManager::unloadAllPlugins() {
    for (auto& plugin : m_plugins) {
        unloadPlugin(plugin.get());
    }
    m_plugins.clear();
    m_extension_map.clear();
}

ISyntaxHighlighter* PluginManager::getHighlighterForFile(const std::string& filename) const {
    std::string extension = getFileExtension(filename);
    
    auto it = m_extension_map.find(extension);
    if (it != m_extension_map.end()) {
        return it->second;
    }
    
    // Try to find by content analysis
    for (const auto& plugin : m_plugins) {
        if (plugin->highlighter && plugin->highlighter->canHighlight(filename, "")) {
            return plugin->highlighter.get();
        }
    }
    
    return nullptr;
}

ISyntaxHighlighter* PluginManager::getHighlighterByName(const std::string& name) const {
    for (const auto& plugin : m_plugins) {
        if (plugin->highlighter && plugin->highlighter->getName() == name) {
            return plugin->highlighter.get();
        }
    }
    return nullptr;
}

std::vector<std::string> PluginManager::getLoadedPluginNames() const {
    std::vector<std::string> names;
    for (const auto& plugin : m_plugins) {
        if (plugin->highlighter) {
            names.push_back(plugin->highlighter->getName());
        }
    }
    return names;
}

bool PluginManager::loadPluginFromHandle(PluginHandle handle, const std::string& filename) {
    auto plugin = std::make_unique<LoadedPlugin>();
    plugin->handle = handle;
    plugin->filename = filename;
    
    // Get plugin functions
    plugin->create_func = reinterpret_cast<CreateHighlighterFunc>(
        getSymbol(handle, CREATE_HIGHLIGHTER_FUNC_NAME));
    plugin->destroy_func = reinterpret_cast<DestroyHighlighterFunc>(
        getSymbol(handle, DESTROY_HIGHLIGHTER_FUNC_NAME));
    
    GetPluginInfoFunc info_func = reinterpret_cast<GetPluginInfoFunc>(
        getSymbol(handle, GET_PLUGIN_INFO_FUNC_NAME));
    
    if (!plugin->create_func || !plugin->destroy_func) {
        std::cerr << "Plugin missing required functions: " << filename << std::endl;
        closeLibrary(handle);
        return false;
    }
    
    // Get plugin info
    if (info_func) {
        plugin->info = info_func();
    }
    
    // Create highlighter instance
    try {
        ISyntaxHighlighter* highlighter = plugin->create_func();
        if (!highlighter) {
            std::cerr << "Failed to create highlighter from plugin: " << filename << std::endl;
            closeLibrary(handle);
            return false;
        }
        
        plugin->highlighter.reset(highlighter);
        
        std::cout << "Loaded plugin: " << highlighter->getName() 
                  << " v" << highlighter->getVersion() << std::endl;
        
        m_plugins.push_back(std::move(plugin));
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception creating highlighter: " << e.what() << std::endl;
        closeLibrary(handle);
        return false;
    }
}

void PluginManager::unloadPlugin(LoadedPlugin* plugin) {
    if (!plugin) return;
    
    if (plugin->highlighter && plugin->destroy_func) {
        plugin->destroy_func(plugin->highlighter.release());
    }
    
    if (plugin->handle) {
        closeLibrary(plugin->handle);
        plugin->handle = nullptr;
    }
}

std::string PluginManager::getFileExtension(const std::string& filename) const {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    
    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

void PluginManager::updateExtensionMap() {
    m_extension_map.clear();
    
    for (const auto& plugin : m_plugins) {
        if (plugin->highlighter) {
            auto extensions = plugin->highlighter->getSupportedExtensions();
            for (const auto& ext : extensions) {
                m_extension_map[ext] = plugin->highlighter.get();
            }
        }
    }
}

// Platform-specific implementations
#ifdef WINDOWS_PLATFORM

PluginHandle PluginManager::openLibrary(const std::string& path) {
    return LoadLibraryA(path.c_str());
}

void PluginManager::closeLibrary(PluginHandle handle) {
    if (handle) {
        FreeLibrary(handle);
    }
}

void* PluginManager::getSymbol(PluginHandle handle, const char* symbol_name) {
    return reinterpret_cast<void*>(GetProcAddress(handle, symbol_name));
}

std::string PluginManager::getLastError() {
    DWORD error = GetLastError();
    if (error == 0) return "";
    
    LPSTR message_buffer = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);
    
    std::string message(message_buffer);
    LocalFree(message_buffer);
    return message;
}

#else // Linux/Unix

PluginHandle PluginManager::openLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

void PluginManager::closeLibrary(PluginHandle handle) {
    if (handle) {
        dlclose(handle);
    }
}

void* PluginManager::getSymbol(PluginHandle handle, const char* symbol_name) {
    return dlsym(handle, symbol_name);
}

std::string PluginManager::getLastError() {
    const char* error = dlerror();
    return error ? std::string(error) : "";
}

#endif

} // namespace subzero