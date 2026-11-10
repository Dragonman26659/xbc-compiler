#pragma once
#include <string>

// Prints a warning message to stderr
void WARN(const std::string& message);
// Prints an error message to stderr and throws an exception
[[noreturn]] void ERROR(const std::string& message);