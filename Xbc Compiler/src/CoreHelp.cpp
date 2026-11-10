#include "CoreHelp.h"
#include "stdio.h"

#include <stdexcept>

// Prints a warning message to stderr
void WARN(const std::string& message) {
    fprintf(stderr, "WARNING: %s\n", message.c_str());
}

// Prints an error message to stderr and throws an exception
[[noreturn]] void ERROR(const std::string& message) {
    fprintf(stderr, "ERROR: %s\n", message.c_str());
    throw std::runtime_error(message);
}