#ifndef COMPAT_H
#define COMPAT_H

// Compatibility header for C++98/gnu++0x
// Provides fallbacks for modern C++ features

#include <memory>
#include <stdint.h>
#include <sstream>

// Define char32_t for C++98 compatibility
#if __cplusplus < 201103L
typedef uint32_t char32_t;
#endif

// Check if we have C++11 smart pointers
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
    // We have C++11 or gnu++0x, use standard smart pointers
    using std::unique_ptr;
    using std::shared_ptr;
    using std::make_shared;
#else
    // Fallback to tr1 or custom implementations
    #ifdef __GNUC__
        // GCC has tr1 support
        #include <tr1/memory>
        using std::tr1::shared_ptr;
        // For unique_ptr, we'll use auto_ptr as a fallback
        #define unique_ptr auto_ptr
    #else
        // For other compilers, use auto_ptr as fallback
        #include <memory>
        #define unique_ptr auto_ptr
        #define shared_ptr auto_ptr
    #endif
#endif

// NULL compatibility
#ifndef NULL
#define NULL 0
#endif

// Simple to_string replacement for C++98
namespace compat {
    template<typename T>
    std::string to_string(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
    
    // Simple stoi replacement for C++98
    inline int stoi(const std::string& str) {
        std::istringstream iss(str);
        int value;
        iss >> value;
        return value;
    }
}

#endif // COMPAT_H