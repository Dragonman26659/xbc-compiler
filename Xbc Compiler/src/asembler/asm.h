#pragma once
#include "instructionset.h"
#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>


using namespace std;

// maps labels to their corresponding line numbers in the code section
struct label {
	int codeIndex;			// holds the index to the instruction in the code vector
	int currentIndex;		// holds the current index to the instruction in the final instructions (updates as operations are expanded)

	uint8_t instructionIndex8;
	uint16_t instructionIndex16;
};

// maps data labels to their corresponding addresses in main memory
enum DATATYPE {
	WORD,
	SWORD,
	FWORD
};

struct asmData {
	uint8_t bitdata8;			// contains preinitalised values for 8 bits
	uint16_t bitdata16;			// contains preinitalised values for 16 bits

	bool initalised = false;	// if false then data is not initalised and is just a "pointer"
	DATATYPE type;				// type of data (signed, unsigned, float)
	int RelativeLocation;		// location from the end of program until the word this data is stored in
};



// final instruction buffer that will be written to the output file
struct Instruction {
	INSTRUCTIONS opcode;
	string operandA;
	string operandB;

	uint8_t bitdata8;			// contains final instruction for 8 bits
	uint16_t bitdata16;			// contains final instruction for 16 bits
};


void assemble(const std::string& inputFile, const std::string& outputFile);