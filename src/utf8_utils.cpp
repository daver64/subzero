#include "utf8_utils.h"
#include <cstdint>

namespace subzero {
namespace utf8 {

size_t length(const std::string& str) {
    size_t len = 0;
    for (size_t i = 0; i < str.length(); ) {
        size_t char_len = charByteLength(str, i);
        if (char_len == 0) break; // Invalid UTF-8
        i += char_len;
        ++len;
    }
    return len;
}

size_t charByteLength(const std::string& str, size_t pos) {
    if (pos >= str.length()) return 0;
    
    uint8_t byte = static_cast<uint8_t>(str[pos]);
    
    if (byte < 0x80) return 1;      // 0xxxxxxx
    if (byte < 0xC0) return 0;      // Invalid start byte
    if (byte < 0xE0) return 2;      // 110xxxxx
    if (byte < 0xF0) return 3;      // 1110xxxx
    if (byte < 0xF8) return 4;      // 11110xxx
    
    return 0; // Invalid
}

std::string charAt(const std::string& str, size_t char_pos) {
    size_t byte_pos = charToByte(str, char_pos);
    if (byte_pos >= str.length()) return "";
    
    size_t char_len = charByteLength(str, byte_pos);
    if (char_len == 0) return "";
    
    return str.substr(byte_pos, char_len);
}

size_t nextCharacter(const std::string& str, size_t byte_pos) {
    if (byte_pos >= str.length()) return str.length();
    
    size_t char_len = charByteLength(str, byte_pos);
    if (char_len == 0) return byte_pos + 1; // Skip invalid byte
    
    return byte_pos + char_len;
}

size_t prevCharacter(const std::string& str, size_t byte_pos) {
    if (byte_pos == 0) return 0;
    
    --byte_pos;
    
    // Move backward until we find a valid UTF-8 start byte
    while (byte_pos > 0 && (static_cast<uint8_t>(str[byte_pos]) & 0xC0) == 0x80) {
        --byte_pos;
    }
    
    return byte_pos;
}

size_t charToByte(const std::string& str, size_t char_pos) {
    size_t byte_pos = 0;
    size_t current_char = 0;
    
    while (byte_pos < str.length() && current_char < char_pos) {
        size_t char_len = charByteLength(str, byte_pos);
        if (char_len == 0) {
            ++byte_pos; // Skip invalid byte
        } else {
            byte_pos += char_len;
        }
        ++current_char;
    }
    
    return byte_pos;
}

size_t byteToChar(const std::string& str, size_t byte_pos) {
    size_t char_pos = 0;
    size_t current_byte = 0;
    
    while (current_byte < byte_pos && current_byte < str.length()) {
        size_t char_len = charByteLength(str, current_byte);
        if (char_len == 0) {
            ++current_byte; // Skip invalid byte
        } else {
            current_byte += char_len;
        }
        ++char_pos;
    }
    
    return char_pos;
}

bool isValid(const std::string& str) {
    for (size_t i = 0; i < str.length(); ) {
        size_t char_len = charByteLength(str, i);
        if (char_len == 0) return false;
        
        // Check that we have enough bytes
        if (i + char_len > str.length()) return false;
        
        // Check continuation bytes
        for (size_t j = 1; j < char_len; ++j) {
            if ((static_cast<uint8_t>(str[i + j]) & 0xC0) != 0x80) {
                return false;
            }
        }
        
        i += char_len;
    }
    return true;
}

bool isValidChar(const std::string& str, size_t pos) {
    size_t char_len = charByteLength(str, pos);
    if (char_len == 0 || pos + char_len > str.length()) return false;
    
    // Check continuation bytes
    for (size_t j = 1; j < char_len; ++j) {
        if ((static_cast<uint8_t>(str[pos + j]) & 0xC0) != 0x80) {
            return false;
        }
    }
    
    return true;
}

std::string substr(const std::string& str, size_t char_start, size_t char_length) {
    size_t byte_start = charToByte(str, char_start);
    if (byte_start >= str.length()) return "";
    
    if (char_length == std::string::npos) {
        return str.substr(byte_start);
    }
    
    size_t byte_end = charToByte(str, char_start + char_length);
    return str.substr(byte_start, byte_end - byte_start);
}

} // namespace utf8
} // namespace subzero