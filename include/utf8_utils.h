#pragma once
#include <string>
#include <cstdint>

namespace subzero {
namespace utf8 {

// Get the length of a UTF-8 string in characters (not bytes)
size_t length(const std::string& str);

// Get the byte length of a single UTF-8 character starting at position
size_t charByteLength(const std::string& str, size_t pos);

// Get character at position (returns UTF-8 sequence as string)
std::string charAt(const std::string& str, size_t char_pos);

// Move to next character position (returns byte position)
size_t nextCharacter(const std::string& str, size_t byte_pos);

// Move to previous character position (returns byte position)
size_t prevCharacter(const std::string& str, size_t byte_pos);

// Convert character position to byte position
size_t charToByte(const std::string& str, size_t char_pos);

// Convert byte position to character position
size_t byteToChar(const std::string& str, size_t byte_pos);

// Validate UTF-8 sequence
bool isValid(const std::string& str);

// Validate a single UTF-8 character starting at position
bool isValidChar(const std::string& str, size_t pos);

// Get substring by character positions (not byte positions)
std::string substr(const std::string& str, size_t char_start, size_t char_length = std::string::npos);

} // namespace utf8
} // namespace subzero