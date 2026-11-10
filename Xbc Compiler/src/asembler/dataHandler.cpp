#include "datahandler.h"
#include "../CoreHelp.h"
#include "../../../common/float16.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

vector<string> splitLine(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;
    while (ss >> token) tokens.push_back(token);
    return tokens;
}

namespace {
long long ParseInteger(const string& text) {
    size_t start = 0;
    bool negative = false;
    if (!text.empty() && (text[0] == '-' || text[0] == '+')) {
        negative = text[0] == '-'; start = 1;
    }
    int base = 10;
    if (text.size() >= start + 2 && text[start] == '0') {
        char prefix = text[start+1];
        if (prefix == 'x' || prefix == 'X') { base = 16; start += 2; }
        else if (prefix == 'b' || prefix == 'B') { base = 2; start += 2; }
    }
    if (start == text.size()) ERROR("Invalid integer: " + text);
    string digits = text.substr(start);
    for (char c : digits) {
        int digit = c >= '0' && c <= '9' ? c-'0' :
                    c >= 'a' && c <= 'f' ? c-'a'+10 :
                    c >= 'A' && c <= 'F' ? c-'A'+10 : -1;
        if (digit < 0 || digit >= base) ERROR("Invalid integer: " + text);
    }
    long long value;
    try { value = stoll(digits, nullptr, base); }
    catch (const std::exception&) { ERROR("Integer out of range: " + text); }
    return negative ? -value : value;
}
uint16_t ConvertInteger(const string& text, DATATYPE type, unsigned bits) {
    if (type != WORD && type != SWORD) ERROR("Invalid integer data type");
    long long value = ParseInteger(text);
    long long minimum = type == SWORD ? -(1LL << (bits-1)) : 0;
    long long maximum = type == SWORD ? (1LL << (bits-1))-1 : (1LL << bits)-1;
    if (value < minimum || value > maximum) ERROR("Integer out of range: " + text);
    return static_cast<uint16_t>(value);
}
}

uint8_t Convert8Bit(string inputString, DATATYPE type) {
    return static_cast<uint8_t>(ConvertInteger(inputString, type, 8));
}
uint16_t Convert16Bit(string inputString, DATATYPE type) {
    if (type != FWORD) return ConvertInteger(inputString, type, 16);
    size_t consumed = 0;
    double value;
    if (inputString.empty() || std::any_of(inputString.begin(), inputString.end(),
            [](unsigned char c) { return std::isspace(c) != 0; }))
        ERROR("Invalid float: " + inputString);
    try { value = std::stod(inputString, &consumed); }
    catch (const std::exception&) { ERROR("Invalid float: " + inputString); }
    if (consumed != inputString.size()) ERROR("Invalid float: " + inputString);
    return xbc::EncodeFloat16(value);
}
