#pragma once
#include <cstdint>
#include <cmath>
#include <limits>

namespace xbc {
inline double DecodeFloat16(uint16_t bits) {
    bool isNegative = (bits & 0x8000) != 0;
    unsigned exponent = (bits >> 10) & 0x1F;
    unsigned fraction = bits & 0x03FF;

    double value = 0;

    if (exponent == 0) {
        value = std::ldexp(static_cast<double>(fraction), -24);
    }
    else if (exponent == 31) {
        if (fraction == 0) {
            value = std::numeric_limits<double>::infinity();
        }
        else {
            value = std::numeric_limits<double>::quiet_NaN();
        }
    }
    else {
        value = std::ldexp(static_cast<double>(1024 + fraction),
            static_cast<int>(exponent) - 25);
    }

    if (isNegative) {
        return -value;
    }

    return value;
}


inline unsigned RoundToNearestEven(double value) {
    double wholeValue = std::floor(value);
    unsigned roundedValue = static_cast<unsigned>(wholeValue);
    double fraction = value - wholeValue;

    if (fraction > 0.5 || (fraction == 0.5 && (roundedValue & 1u))) {
        ++roundedValue;
    }

    return roundedValue;
}


inline uint16_t EncodeFloat16(double value) {
    uint16_t signBit = 0;

    if (std::signbit(value)) {
        signBit = 0x8000;
    }

    double magnitude = std::fabs(value);

    if (std::isnan(value)) {
        return 0x7E00;
    }

    if (std::isinf(value)) {
        return static_cast<uint16_t>(signBit | 0x7C00);
    }

    if (magnitude == 0) {
        return signBit;
    }

    if (magnitude < std::ldexp(1.0, -14)) {
        unsigned fraction = RoundToNearestEven(std::ldexp(magnitude, 24));
        return static_cast<uint16_t>(signBit | fraction);
    }

    int exponent = 0;
    std::frexp(magnitude, &exponent);
    --exponent;

    if (exponent > 15) {
        return static_cast<uint16_t>(signBit | 0x7C00);
    }

    unsigned significand = RoundToNearestEven(std::ldexp(magnitude, 10 - exponent));

    if (significand == 2048) {
        significand = 1024;
        ++exponent;
    }

    if (exponent > 15) {
        return static_cast<uint16_t>(signBit | 0x7C00);
    }

    return static_cast<uint16_t>(signBit | ((exponent + 15) << 10) |
        (significand - 1024));
}



} // namespace xbc
