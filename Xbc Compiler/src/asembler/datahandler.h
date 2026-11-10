#pragma once
#include "asm.h"
#include <sstream>

using namespace std;


vector<string> splitLine(const string& line);

uint8_t Convert8Bit(string inputString, DATATYPE type);
uint16_t Convert16Bit(string inputString, DATATYPE type);