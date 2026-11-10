#include <cstdint>
#include "asm.h"
#include "datahandler.h"
#include "../CoreHelp.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include "../../../common/float16.h"

bool is16bit = false;
int instructionCount = 0;

vector<string> rawCode;
vector<string> dataSection;
vector<string> codeSection;

map<string, label> labels;
// Internal labels may name a private immediate operand inside an instruction.
map<string, size_t> internalLabelWordOffsets;
map<int, int> codeToInst;

map<string, asmData> dataMap;

vector<Instruction> instructionBuffer;

vector<uint8_t> final8b;
vector<uint16_t> final16b;

// REGISTER BULLSHIT
namespace {

	const string addressRegister = "r14";
	map<string, size_t> symbolAddresses;
	map<string, uint16_t> dataWordValues;
	size_t codeWordCount = 0;
	size_t internalLabelCount = 0;
	long long ParseNumber(const string& inputString);
	unsigned GetRegisterIndex(const string& inputString, unsigned maximumRegisterIndex);
	string ToUpper(string inputString) {
		for (char& character : inputString) {
			if (character >= 'a' && character <= 'z') {
				character = static_cast<char>(character - 'a' + 'A');
			}
		}
		return inputString;
	}


	bool isRegisterName(const string& inputString) {
		string registerName = ToUpper(inputString);

		if (registerName == "EAX" || registerName == "EBX" || registerName == "ECX" || registerName == "EDX") {
			return true;
		}
		if (registerName.size() < 2 || registerName[0] != 'R') {
			return false;
		}
		for (size_t characterIndex = 1; characterIndex < registerName.size(); ++characterIndex) {
			char character = registerName[characterIndex];

			if (character < '0' || character > '9') {
				return false;
			}
		}
		return true;
	}


	bool IsIdentifierLetter(char character) {
		return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
	}


	bool IsValidIdentifier(const string& inputString) {
		if (inputString.empty() || !IsIdentifierLetter(inputString[0]) || inputString.rfind("__asm_", 0) == 0 || isRegisterName(inputString)) {
			return false;
		}
		for (char character : inputString) {
			bool isDigit = character >= '0' && character <= '9';

			if (!IsIdentifierLetter(character) && !isDigit) {
				return false;
			}
		}
		return true;
	}


	void ValidateUserRegister(const string& inputString, unsigned maximumValue) {
		if (!isRegisterName(inputString)) {
			ERROR("Expected register, got: " + inputString);
		}


		unsigned registerIndex = GetRegisterIndex(inputString, maximumValue);

        (void)registerIndex; // All sixteen hardware registers are addressable.
	}


	uint16_t ConvertIntegerData(const string& inputString, DATATYPE type) {
		long long parsedValue = ParseNumber(inputString);
		long long minimumValue = 0;
		long long maximumValue = 255;

		if (is16bit) {
			maximumValue = 65535;
		}
		if (type == SWORD) {
			minimumValue = -128;
			maximumValue = 127;

			if (is16bit) {
				minimumValue = -32768;
				maximumValue = 32767;
			}
		}
		if (parsedValue < minimumValue || parsedValue > maximumValue) {
			ERROR("Data value out of range: " + inputString);
		}
		return static_cast<uint16_t>(parsedValue);
	}


    uint16_t ConvertFloatData(const string& inputString) {
        if (!is16bit) ERROR("FWORD requires 16BC");
        return Convert16Bit(inputString, FWORD);
    }

	void ResetAssembly() {
		is16bit = false;
		instructionCount = 0;
		codeWordCount = 0;
		internalLabelCount = 0;
		rawCode.clear();
		dataSection.clear();
		codeSection.clear();
		labels.clear();
        internalLabelWordOffsets.clear();
		codeToInst.clear();
		dataMap.clear();
		dataWordValues.clear();
		symbolAddresses.clear();
		instructionBuffer.clear();
		final8b.clear();
		final16b.clear();
	}
}

//============================================
//			PREPROCESSING
//============================================

void stripComments(string& line) {
	size_t position = line.find(';');

	if (position != string::npos) {
		line.erase(position);
	}
}

void StripWhitespace(string& line) {
	size_t firstCharacter = line.find_first_not_of(" \t\r\n");

	if (firstCharacter == string::npos) {
		line.clear();
		return;
	}
	size_t lastCharacter = line.find_last_not_of(" \t\r\n");
	line = line.substr(firstCharacter, lastCharacter - firstCharacter + 1);
}

void processLine(string& line) {
	stripComments(line);
	StripWhitespace(line);

	if (!line.empty()) {
		rawCode.push_back(line);
	}
}

void ProsessFile(const string& inputFile) {
	ifstream input(inputFile);

	if (!input) {
		ERROR("Cannot open input file: " + inputFile);
	}


	string line;

	while (getline(input, line)) {
		processLine(line);
	}
	if (input.bad()) {
		ERROR("Failed reading input file: " + inputFile);
	}
}

void SplitSegments() {
	enum Section {
		NONE, DATA, CODE
	}
	section = NONE;

	for (const string& line : rawCode) {
		string directive = ToUpper(line);

		if (directive == ".DATA") {
			section = DATA;
		}
		else if (directive == ".CODE") {
			section = CODE;
		}
		else if (section == DATA) {
			dataSection.push_back(line);
		}
		else if (section == CODE) {
			codeSection.push_back(line);
		}
		else {
			ERROR("Content outside .data/.code: " + line);
		}
	}
}

//============================================
//				DATA SECTION
//============================================

void MapData() {
	int offset = 0;

	for (const string& line : dataSection) {
		auto tokens = splitLine(line);

		if (tokens.size() != 2 && tokens.size() != 3) {
			ERROR("Expected TYPE name [value]: " + line);
		}


		string typeName = ToUpper(tokens[0]);
		DATATYPE type;

		if (typeName == "WORD") {
			type = WORD;
		}
		else if (typeName == "SWORD") {
			type = SWORD;
		}
		else if (typeName == "FWORD") {
			type = FWORD;
		}
		else {
			ERROR("Unknown data type: " + tokens[0]);
			return;
		}
		if (type == FWORD && !is16bit) {
			ERROR("FWORD requires 16BC");
		}
		const string& symbolName = tokens[1];

		if (!IsValidIdentifier(symbolName) || dataMap.count(symbolName)) {
			ERROR("Invalid/duplicate data name: " + symbolName);
		}


		uint16_t wordValue = 0;

		if (tokens.size() == 3) {
			if (type == FWORD) {
				wordValue = ConvertFloatData(tokens[2]);
			}
			else {
				wordValue = ConvertIntegerData(tokens[2], type);
			}
		}
		asmData mappedEntry{};
		mappedEntry.type = type;
		mappedEntry.RelativeLocation = offset++;
		mappedEntry.initalised = tokens.size() == 3;

		// dataBits is the authoritative unsigned representation.
		mappedEntry.bitdata8 = static_cast<uint8_t>(wordValue);
		mappedEntry.bitdata16 = wordValue;
		dataMap.emplace(symbolName, mappedEntry);
		dataWordValues.emplace(symbolName, wordValue);
	}
}

//============================================
//			INSTRUCTION GENERATION
//============================================

void emit(INSTRUCTIONS opcode, const string& operandA = "", const string& operandB = "") {
	Instruction instruction{};
	instruction.opcode = opcode;
	instruction.operandA = operandA;
	instruction.operandB = operandB;
	instructionBuffer.push_back(instruction);
}

void defineLabel(const string& symbolName, bool internal = false) {
	if ((!internal && !IsValidIdentifier(symbolName)) || labels.count(symbolName) || dataMap.count(symbolName)) {
		ERROR("Invalid/duplicate label: " + symbolName);
	}
	label mappedEntry{};
	mappedEntry.currentIndex = static_cast<int>(instructionBuffer.size());
	labels.emplace(symbolName, mappedEntry);
}

string CreateInternalLabel() {
	return "__asm_" + to_string(internalLabelCount++);
}

void ValidateSourceOperandCount(const vector<string>& tokens, size_t operandCount, const string& sourceLine) {
	if (tokens.size() != operandCount + 1) {
		ERROR("Wrong operand count: " + sourceLine);
	}
}

INSTRUCTIONS HaltOpcode() {
	if (is16bit) {
		return HLT16;
	}
	return HLT8;
}

INSTRUCTIONS BranchOpcode(const string& mnemonic) {
	if (mnemonic == "BRA") {
		return BRA;
	}
	if (mnemonic == "BRZ") {
		return BRZ;
	}
	if (mnemonic == "BRN") {
		return BRN;
	}
	return BRP;
}

void StackOperand(INSTRUCTIONS operation, const string& reg) {
    ValidateUserRegister(reg, 15);
    unsigned index = GetRegisterIndex(reg, 15);
    if (index < 4) { emit(operation, reg); return; }
    string scratch = index == 14 ? "r14" : "r15";
    emit(STR, "EAX"); emit(STA, scratch);
    if (operation == PUSH) { emit(STR, reg); emit(STA, "EAX"); emit(PUSH, "EAX"); }
    else { emit(POP, "EAX"); emit(STR, "EAX"); emit(STA, reg); }
    emit(STR, scratch); emit(STA, "EAX");
}

// Memory transport through EAX; temporary stack slot preserves it.
void ReadToRegister(const string& destination, const string& address, bool indirect) {
    ValidateUserRegister(destination, 15);
    bool preserve = GetRegisterIndex(destination, 15) != 0;
    if (preserve) emit(PUSH, "EAX");
    if (indirect) { ValidateUserRegister(address, 15); emit(STR, address); }
    else { emit(LDA_IMM, addressRegister, address); emit(STR, addressRegister); }
    emit(RRAM, "EAX");
    if (preserve) {
        emit(STR, "EAX"); emit(POP, "EAX"); emit(STA, destination);
    }
}
void WriteFromRegister(const string& source, const string& address, bool indirect) {
    ValidateUserRegister(source, 15);
    // The hardware WRAM field only addresses EAX..EDX.
    if (GetRegisterIndex(source, 15) < 4) {
        if (indirect) { ValidateUserRegister(address, 15); emit(STR, address); }
        else { emit(LDA_IMM, addressRegister, address); emit(STR, addressRegister); }
        emit(WRAM, source);
    } else {
        string transport = indirect && GetRegisterIndex(address, 15) == 0 ? "EDX" : "EAX";
        string actualAddress = address;
        if (indirect && GetRegisterIndex(address, 15) == 15) {
            actualAddress = GetRegisterIndex(source, 15) == 13 ? "r15" : "r14";
            emit(STR, address); emit(STA, actualAddress);
        }
        emit(STR, source); emit(PUSH, transport); emit(STA, transport);
        if (indirect) emit(STR, actualAddress);
        else { emit(LDA_IMM, addressRegister, address); emit(STR, addressRegister); }
        emit(WRAM, transport); emit(POP, transport);
    }
}

void GenerateInstructions() {
	const map<string, INSTRUCTIONS> binaryOperations = {
		{"ADD", ADD},
		{"SUB", SUB},
		{"AND", AND},
		{"OR", OR},
		{"XOR", XOR}
	};
	const map<string, INSTRUCTIONS> extendedOperations = {
		{"MULT", SMULT},
		{"DIV", SDIV},
		{"UMULT", MULT},
		{"UDIV", DIV},
		{"FMULT", FLMULT},
		{"FDIV", FLDIV},
		{"FADD", FLADD},
		{"FSUB", FLSUB},
		{"SMULT", SMULT},
		{"SDIV", SDIV},
		{"FLADD", FLADD},
		{"FLSUB", FLSUB},
		{"FLMULT", FLMULT},
		{"FLDIV", FLDIV}
	};
	const map<string, INSTRUCTIONS> conversionOperations = {
		{"ITOF", ITOF},
		{"UTOF", UTOF},
		{"FTOI", FTOI},
		{"FTOU", FTOU}
	};

	for (size_t sourceIndex = 0; sourceIndex < codeSection.size(); ++sourceIndex) {
		string line = codeSection[sourceIndex];
		// Support both "name:" and "name: instruction".
		size_t labelSeparator = line.find(':');

		if (labelSeparator != string::npos) {
			string symbolName = line.substr(0, labelSeparator);
			StripWhitespace(symbolName);
			defineLabel(symbolName);
			line.erase(0, labelSeparator + 1);
			StripWhitespace(line);
		}
		if (line.empty()) {
			continue;
		}
		std::replace(line.begin(), line.end(), ',', ' ');
		auto tokens = splitLine(line);
		string mnemonic = ToUpper(tokens[0]);
		if (mnemonic == "COMP") mnemonic = "CMP";
		if (mnemonic == "MUL") mnemonic = "MULT";
		codeToInst[static_cast<int>(sourceIndex)] = static_cast<int>(instructionBuffer.size());
		auto binaryOperation = binaryOperations.find(mnemonic);
		auto extendedOperation = extendedOperations.find(mnemonic);
		auto conversionOperation = conversionOperations.find(mnemonic);

		if (binaryOperation != binaryOperations.end()) {
			ValidateSourceOperandCount(tokens, 2, line);
			ValidateUserRegister(tokens[1], 3);
			ValidateUserRegister(tokens[2], 3);
			emit(binaryOperation->second, tokens[1], tokens[2]);
			emit(STA, tokens[1]);
		}
		else if (extendedOperation != extendedOperations.end()) {
			if (!is16bit) {
				ERROR("Instruction requires 16BC: " + mnemonic);
			}
			ValidateSourceOperandCount(tokens, 2, line);
			ValidateUserRegister(tokens[1], 15);
			ValidateUserRegister(tokens[2], 15);
			emit(extendedOperation->second, tokens[1], tokens[2]);
			emit(STA, tokens[1]);
		}
		else if (conversionOperation != conversionOperations.end()) {
			if (!is16bit) {
				ERROR("Instruction requires 16BC: " + mnemonic);
			}
			ValidateSourceOperandCount(tokens, 2, line);
			ValidateUserRegister(tokens[1], 15);
			ValidateUserRegister(tokens[2], 15);
			emit(conversionOperation->second, tokens[1], tokens[2]);
		}
		else if (mnemonic == "CMP" || mnemonic == "FCMP") {
			ValidateSourceOperandCount(tokens, 2, line);

			if (mnemonic == "FCMP" && !is16bit) {
				ERROR("FCMP requires 16BC");
			}


			unsigned maximumValue = 3;

			if (mnemonic == "FCMP") {
				maximumValue = 15;
			}
			ValidateUserRegister(tokens[1], maximumValue);
			ValidateUserRegister(tokens[2], maximumValue);

			if (mnemonic == "CMP") {
				emit(SUB, tokens[1], tokens[2]);
			}
			else {
				emit(FCMP, tokens[1], tokens[2]);
			}
		}
        else if (mnemonic == "MOV") {
            ValidateSourceOperandCount(tokens, 2, line);
            string a = tokens[1], b = tokens[2];
            auto bracketed = [](const string& operand) {
                return operand.size() >= 3 && operand.front() == '[' && operand.back() == ']';
            };
            if (isRegisterName(a)) {
                ValidateUserRegister(a, 15);
                if (isRegisterName(b)) {
                    ValidateUserRegister(b, 15); emit(STR, b); emit(STA, a);
                } else if (bracketed(b)) {
                    string inner = b.substr(1, b.size() - 2);
                    if (isRegisterName(inner)) ReadToRegister(a, inner, true);
                    else { if (!IsValidIdentifier(inner)) ParseNumber(inner); emit(LDA_IMM, a, inner); }
                } else if (dataMap.count(b)) ReadToRegister(a, b, false);
                else emit(LDA_IMM, a, b);
            } else {
                bool indirect = false;
                if (bracketed(a)) { a = a.substr(1, a.size() - 2); indirect = isRegisterName(a); }
                if (!indirect && !dataMap.count(a)) ParseNumber(a);
                if (!isRegisterName(b)) ERROR("Memory MOV source must be a register");
                WriteFromRegister(b, a, indirect);
            }
        }
		else if (mnemonic == "BRA" || mnemonic == "BRZ" || mnemonic == "BRN" || mnemonic == "BRP") {
			ValidateSourceOperandCount(tokens, 1, line);
			INSTRUCTIONS opcode = BranchOpcode(mnemonic);

			if (isRegisterName(tokens[1])) {
				ValidateUserRegister(tokens[1], 15);
				emit(opcode, tokens[1]);
			}
			else {
				emit(LDA_IMM, addressRegister, tokens[1]);
				emit(opcode, addressRegister);
			}
		}
		else if (mnemonic == "NOT") {
			ValidateSourceOperandCount(tokens, 1, line);
			ValidateUserRegister(tokens[1], 15);
			emit(NOT, tokens[1]);
			emit(STA, tokens[1]);
		}
        else if (mnemonic == "CALL") {
            ValidateSourceOperandCount(tokens, 1, line);
            if (isRegisterName(tokens[1])) ERROR("CALL requires a label/address");
            string returnLabel = CreateInternalLabel();
            emit(STR, "EAX"); emit(STA, "r15");
            emit(LDA_IMM, "EAX", returnLabel); emit(PUSH, "EAX");
            emit(STR, "r15"); emit(STA, "EAX");
            emit(LDA_IMM, addressRegister, tokens[1]); emit(BRA, addressRegister);
            defineLabel(returnLabel, true);
        }
        else if (mnemonic == "IRET") {
            ValidateSourceOperandCount(tokens, 0, line);
            if (!is16bit) ERROR("IRET requires .16bc and a 15-word device interrupt frame");
            // Keep interrupts masked throughout. Patch two private immediate
            // words, avoiding the R14 clobber in ordinary POP/RET macros.
            // Executable RAM is writable; each IRET expansion owns its slots.
            string savedEAX = CreateInternalLabel();
            string returnPC = CreateInternalLabel();
            emit(POP, "EAX");
            emit(LDA_IMM, "r15", savedEAX);
            emit(STR, "r15"); emit(WRAM, "EAX");
            emit(POP, "EBX"); emit(POP, "ECX"); emit(POP, "EDX");
            for (unsigned reg = 5; reg <= 14; ++reg) {
                emit(POP, "EAX"); emit(STR, "EAX");
                emit(STA, "r" + to_string(reg));
            }
            emit(POP, "EAX");
            emit(LDA_IMM, "r15", returnPC);
            emit(STR, "r15"); emit(WRAM, "EAX");
            defineLabel(savedEAX, true);
            internalLabelWordOffsets.emplace(savedEAX, 1);
            emit(LDA_IMM, "EAX", "0");
            defineLabel(returnPC, true);
            internalLabelWordOffsets.emplace(returnPC, 1);
            emit(LDA_IMM, "r15", "0");
            emit(BRA, "r15");
        }
        else if (mnemonic == "RET") {
            ValidateSourceOperandCount(tokens, 0, line);
            emit(STR, "EAX"); emit(STA, "r15"); emit(POP, "EAX");
            emit(STR, "EAX"); emit(STA, addressRegister);
            emit(STR, "r15"); emit(STA, "EAX"); emit(BRA, addressRegister);
        }
        else if (mnemonic == "PSH" || mnemonic == "PUSH" || mnemonic == "POP") {
            ValidateSourceOperandCount(tokens, 1, line);
            StackOperand(mnemonic == "POP" ? POP : PUSH, tokens[1]);
        }
		else if (mnemonic == "SFTL" || mnemonic == "SFTR") {
			ValidateSourceOperandCount(tokens, 2, line);
			ValidateUserRegister(tokens[1], 3);
			ValidateUserRegister(tokens[2], 3);
			unsigned destination = GetRegisterIndex(tokens[1], 3);
			vector<string> temporaryRegisters;
			const vector<string> firstFour = {
				"EAX", "EBX", "ECX", "EDX"
			};

			for (unsigned registerIndex = 0; registerIndex < 4; ++registerIndex) {
				if (registerIndex != destination) {
					temporaryRegisters.push_back(firstFour[registerIndex]);
				}
			}


			string shiftCounterRegister = temporaryRegisters[0], shiftConstantRegister = temporaryRegisters[1];
			string loopLabel = CreateInternalLabel(), endLabel = CreateInternalLabel();
			emit(PUSH, shiftCounterRegister);
			emit(PUSH, shiftConstantRegister);
			emit(STR, tokens[2]);
			emit(STA, shiftCounterRegister);
			emit(LDA_IMM, shiftConstantRegister, "0");
			emit(SUB, shiftCounterRegister, shiftConstantRegister);
			emit(LDA_IMM, addressRegister, endLabel);
			emit(BRZ, addressRegister);
			emit(LDA_IMM, shiftConstantRegister, "1");
			defineLabel(loopLabel, true);

			if (mnemonic == "SFTL") {
				emit(LSHIFT, tokens[1]);
			}
			else {
				emit(RSHIFT, tokens[1]);
			}
			emit(SUB, shiftCounterRegister, shiftConstantRegister);
			emit(STA, shiftCounterRegister);
			emit(LDA_IMM, addressRegister, endLabel);
			emit(BRZ, addressRegister);
			emit(LDA_IMM, addressRegister, loopLabel);
			emit(BRA, addressRegister);
			defineLabel(endLabel, true);
			emit(POP, shiftConstantRegister);
			emit(POP, shiftCounterRegister);
		}
		else if (mnemonic == "LSHIFT" || mnemonic == "RSHIFT") {
			ValidateSourceOperandCount(tokens, 1, line);
			ValidateUserRegister(tokens[1], 3);

			if (mnemonic == "LSHIFT") {
				emit(LSHIFT, tokens[1]);
			}
			else {
				emit(RSHIFT, tokens[1]);
			}
		}
		else if (mnemonic == "LDA_IMM") {
			ValidateSourceOperandCount(tokens, 2, line);
			ValidateUserRegister(tokens[1], 15);
			emit(LDA_IMM, tokens[1], tokens[2]);
		}
		else if (mnemonic == "STA" || mnemonic == "STR") {
			ValidateSourceOperandCount(tokens, 1, line);
			ValidateUserRegister(tokens[1], 15);

			if (mnemonic == "STA") {
				emit(STA, tokens[1]);
			}
			else {
				emit(STR, tokens[1]);
			}
		}
		else if (mnemonic == "RRAM" || mnemonic == "WRAM") {
			ValidateSourceOperandCount(tokens, 1, line);
			ValidateUserRegister(tokens[1], 3);

			if (mnemonic == "RRAM") {
				emit(RRAM, tokens[1]);
			}
			else {
				emit(WRAM, tokens[1]);
			}
		}
		else if (mnemonic == "SETINTR" || mnemonic == "SETIRR") {
			ValidateSourceOperandCount(tokens, 1, line);
			emit(SETINTR, tokens[1]);
		}
		else if (mnemonic == "HLT") {
			ValidateSourceOperandCount(tokens, 0, line);
			emit(HaltOpcode());
		}
		else {
			ERROR("Unsupported instruction: " + line);
		}
	}
	// Prevent sequential execution from falling into the data section.
	if (instructionBuffer.empty() || (instructionBuffer.back().opcode != HLT8 && instructionBuffer.back().opcode != HLT16)) {
		emit(HaltOpcode());
	}
	instructionCount = static_cast<int>(instructionBuffer.size());
}

//============================================
//			LABEL AND ADDRESS RESOLUTION
//============================================

void MapLabels() {
	symbolAddresses.clear();
	vector<size_t> instructionAddresses(instructionBuffer.size() + 1);
	size_t wordAddress = 0;

	for (size_t instructionIndex = 0; instructionIndex < instructionBuffer.size(); ++instructionIndex) {
		instructionAddresses[instructionIndex] = wordAddress;

		if (instructionBuffer[instructionIndex].opcode == LDA_IMM) {
			wordAddress += 2;
		}
		else {
			++wordAddress;
		}
	}
	instructionAddresses.back() = wordAddress;
	codeWordCount = wordAddress;

	for (const auto& [symbolName, mappedEntry] : labels) {
		size_t registerIndex = static_cast<size_t>(mappedEntry.currentIndex);

		if (registerIndex >= instructionAddresses.size()) {
			ERROR("Invalid label position: " + symbolName);
		}
		size_t wordOffset = 0;
        auto offset = internalLabelWordOffsets.find(symbolName);
        if (offset != internalLabelWordOffsets.end()) wordOffset = offset->second;
        symbolAddresses.emplace(symbolName, instructionAddresses[registerIndex] + wordOffset);
	}
	for (const auto& [symbolName, mappedEntry] : dataMap) {
		if (!symbolAddresses.emplace(symbolName, wordAddress + static_cast<size_t>(mappedEntry.RelativeLocation)).second) {
			ERROR("Duplicate symbol: " + symbolName);
		}
	}
	size_t memoryCapacity = 200u;

	if (is16bit) {
		memoryCapacity = 59180u;
	}
	if (wordAddress + dataMap.size() > memoryCapacity) {
		ERROR("Code/data exceeds code and heap region");
	}
	for (const auto& [symbolName, address] : symbolAddresses) {
		if (address >= memoryCapacity) {
			ERROR("Address out of range: " + symbolName);
		}
	}
}

void ResolveSymbols() {
	for (Instruction& instruction : instructionBuffer) {
		if (instruction.opcode == LDA_IMM) {
			auto symbolEntry = symbolAddresses.find(instruction.operandB);

			if (symbolEntry != symbolAddresses.end()) {
				instruction.operandB = to_string(symbolEntry->second);
			}
			else {
				ParseNumber(instruction.operandB);
			}
		}
	}
}

namespace {

	// Decimal, 0x hexadecimal and 0b binary. No partial numeric tokens.
	long long ParseNumber(const string& inputString) {
		size_t start = 0;
		bool negative = false;

		if (!inputString.empty() && (inputString[0] == '-' || inputString[0] == '+')) {
			negative = inputString[0] == '-';
			start = 1;
		}
		int base = 10;

		if (inputString.size() >= start + 2 && inputString[start] == '0') {
			char prefix = inputString[start + 1];

			if (prefix == 'x' || prefix == 'X') {
				base = 16;
				start += 2;
			}
			if (prefix == 'b' || prefix == 'B') {
				base = 2;
				start += 2;
			}
		}


		string numberDigits = inputString.substr(start);

		if (numberDigits.empty()) {
			ERROR("Expected a number, got: " + inputString);
		}
		for (char character : numberDigits) {
			int digit = -1;

			if (character >= '0' && character <= '9') {
				digit = character - '0';
			}
			else if (character >= 'a' && character <= 'f') {
				digit = character - 'a' + 10;
			}
			else if (character >= 'A' && character <= 'F') {
				digit = character - 'A' + 10;
			}
			if (digit < 0 || digit >= base) {
				ERROR("Invalid number: " + inputString);
			}
		}


		long long parsedValue = 0;
		try {
			parsedValue = stoll(numberDigits, nullptr, base);
		}
		catch (const std::exception&) {
			ERROR("Number out of range: " + inputString);
		}
		if (negative) {
			return -parsedValue;
		}
		return parsedValue;
	}


	unsigned GetRegisterIndex(const string& inputString, unsigned maximumRegisterIndex) {
		string registerName = inputString;

		for (char& character : registerName) {
			if (character >= 'a' && character <= 'z') {
				character = static_cast<char>(character - 'a' + 'A');
			}
		}


		long long parsedValue = 0;

		if (registerName == "EAX") {
			parsedValue = 0;
		}
		else if (registerName == "EBX") {
			parsedValue = 1;
		}
		else if (registerName == "ECX") {
			parsedValue = 2;
		}
		else if (registerName == "EDX") {
			parsedValue = 3;
		}
		else if (!registerName.empty() && registerName[0] == 'R') {
			// r5 is the fifth register, whose encoded index is 4.
			string numberDigits = registerName.substr(1);

			if (numberDigits.empty()) {
				ERROR("Invalid register: " + inputString);
			}
			for (char character : numberDigits) {
				if (character < '0' || character > '9') {
					ERROR("Invalid register: " + inputString);
				}
			}


			long long registerNumber = ParseNumber(numberDigits);

			if (registerNumber < 1 || registerNumber > 16) {
				ERROR("Expected r1-r16, got: " + inputString);
			}
			parsedValue = registerNumber - 1;
		}
		else {
			// Numeric operands remain accepted as raw indices for internal generation.
			parsedValue = ParseNumber(registerName);
		}
		if (parsedValue < 0 || parsedValue > maximumRegisterIndex) {
			ERROR("Register out of range: " + inputString);
		}
		return static_cast<unsigned>(parsedValue);
	}


	void ValidateInstructionOperands(const Instruction& instruction, int operandCount) {
		bool hasOperandA = !instruction.operandA.empty();
		bool hasOperandB = !instruction.operandB.empty();

		if (hasOperandA != (operandCount >= 1) || hasOperandB != (operandCount == 2)) {
			ERROR("Wrong operand count for hardware opcode " + to_string(static_cast<int>(instruction.opcode)));
		}
	}
} // namespace


//============================================
//			BINARY ENCODING
//============================================

uint16_t EncodeBinaryRegisters(const Instruction& instruction, uint16_t baseOpcode) {
	ValidateInstructionOperands(instruction, 2);
	unsigned registerA = GetRegisterIndex(instruction.operandA, 3);
	unsigned registerB = GetRegisterIndex(instruction.operandB, 3);

	return static_cast<uint16_t>(baseOpcode | (registerA << 3) | (registerB << 1));
}

uint16_t EncodeFourBitRegister(const Instruction& instruction, uint16_t baseOpcode) {
	ValidateInstructionOperands(instruction, 1);
	unsigned registerIndex = GetRegisterIndex(instruction.operandA, 15);

	return static_cast<uint16_t>(baseOpcode | (registerIndex << 1));
}

uint16_t EncodeTwoBitRegister(const Instruction& instruction, uint16_t baseOpcode) {
	ValidateInstructionOperands(instruction, 1);
	unsigned registerIndex = GetRegisterIndex(instruction.operandA, 3);

	return static_cast<uint16_t>(baseOpcode | registerIndex);
}

uint16_t EncodeExtensionRegisters(const Instruction& instruction, uint16_t baseOpcode) {
	if (!is16bit) {
		ERROR("16BC instruction used in 8BC mode");
	}
	ValidateInstructionOperands(instruction, 2);
	unsigned registerA = GetRegisterIndex(instruction.operandA, 15);
	unsigned registerB = GetRegisterIndex(instruction.operandB, 15);

	return static_cast<uint16_t>(baseOpcode | (registerA << 4) | registerB);
}

// Encodes already-expanded hardware instructions. Does not append the data section.
// Legacy instructions retain their original register-field widths in 16BC.
// LDA_IMM uses operandA = register, operandB = resolved literal/address.
void FinalConvert() {
	vector<uint8_t> encoded8;
	vector<uint16_t> encoded16;
	vector<uint16_t> instructionWords;

	for (const Instruction& instruction : instructionBuffer) {
		uint16_t instructionWord = 0;
		uint16_t immediate = 0;
		bool hasImmediate = false;

		switch (instruction.opcode) {
		case LDA_IMM:
		{
			ValidateInstructionOperands(instruction, 2);
			instructionWord = static_cast<uint16_t>(GetRegisterIndex(instruction.operandA, 15));
			long long parsedValue = ParseNumber(instruction.operandB);
			long long minimumValue = -128;
			long long maximumValue = 255;

			if (is16bit) {
				minimumValue = -32768;
				maximumValue = 65535;
			}
			if (parsedValue < minimumValue || parsedValue > maximumValue) {
				ERROR("Immediate out of range");
			}
			immediate = static_cast<uint16_t>(parsedValue);
			hasImmediate = true;
			break;
		}
		case ADD:
			instructionWord = EncodeBinaryRegisters(instruction, 0x80);
			break;

		case SUB:
			instructionWord = EncodeBinaryRegisters(instruction, 0x40);
			break;

		case XOR:
			instructionWord = EncodeBinaryRegisters(instruction, 0xC0);
			break;

		case OR:
			instructionWord = EncodeBinaryRegisters(instruction, 0x20);
			break;

		case AND:
			instructionWord = EncodeBinaryRegisters(instruction, 0x60);
			break;

		case NOT:
			instructionWord = EncodeFourBitRegister(instruction, 0xA0);
			break;

		case BRA:
			instructionWord = EncodeFourBitRegister(instruction, 0x81);
			break;

		case BRZ:
			instructionWord = EncodeFourBitRegister(instruction, 0x41);
			break;

		case BRN:
			instructionWord = EncodeFourBitRegister(instruction, 0xC1);
			break;

		case BRP:
			instructionWord = EncodeFourBitRegister(instruction, 0x21);
			break;

		case STA:
			instructionWord = EncodeFourBitRegister(instruction, 0xA1);
			break;

		case STR:
			instructionWord = EncodeFourBitRegister(instruction, 0x61);
			break;

		case RRAM:
			instructionWord = EncodeTwoBitRegister(instruction, 0xF0);
			break;

		case WRAM:
			instructionWord = EncodeTwoBitRegister(instruction, 0xE8);
			break;

		case LSHIFT:
			instructionWord = EncodeTwoBitRegister(instruction, 0xF8);
			break;

		case RSHIFT:
			instructionWord = EncodeTwoBitRegister(instruction, 0xE4);
			break;

		case POP:
			instructionWord = EncodeTwoBitRegister(instruction, 0xF4);
			break;

		case PUSH:
			instructionWord = EncodeTwoBitRegister(instruction, 0xEC);
			break;

		case SETINTR:
		{
			// Table's AX field : A is enable bit, X is reserved and emitted as 0.
			ValidateInstructionOperands(instruction, 1);
			long long enabled = ParseNumber(instruction.operandA);

			if (enabled != 0 && enabled != 1) {
				ERROR("SETINTR expects 0 or 1");
			}
			instructionWord = static_cast<uint16_t>(0xE0 | (enabled << 1));
			break;
		}
		case HLT8:
		case HLT16:
			ValidateInstructionOperands(instruction, 0);
			instructionWord = 0xFF;

			if (is16bit) {
				instructionWord = 0xFFFF;
			}
			break;

		case MULT:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF800);
			break;

		case DIV:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF400);
			break;

		case FLADD:
			instructionWord = EncodeExtensionRegisters(instruction, 0xFC00);
			break;

		case FLSUB:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF200);
			break;

		case FLMULT:
			instructionWord = EncodeExtensionRegisters(instruction, 0xFA00);
			break;

		case FLDIV:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF600);
			break;

		case SMULT:
			instructionWord = EncodeExtensionRegisters(instruction, 0xFE00);
			break;

		case SDIV:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF100);
			break;

		case FCMP:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF900);
			break;

		case ITOF:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF500);
			break;

		case UTOF:
			instructionWord = EncodeExtensionRegisters(instruction, 0xFD00);
			break;

		case FTOI:
			instructionWord = EncodeExtensionRegisters(instruction, 0xF300);
			break;

		case FTOU:
			instructionWord = EncodeExtensionRegisters(instruction, 0xFB00);
			break;
		default:
			ERROR("Unknown hardware opcode");
		}
		instructionWords.push_back(instructionWord);

		if (is16bit) {
			encoded16.push_back(instructionWord);

			if (hasImmediate) {
				encoded16.push_back(immediate);
			}
		}
		else {
			encoded8.push_back(static_cast<uint8_t>(instructionWord));

			if (hasImmediate) {
				encoded8.push_back(static_cast<uint8_t>(immediate));
			}
		}
	}

	// Commit only after the whole buffer validates successfully.
	final8b = std::move(encoded8);
	final16b = std::move(encoded16);

	for (size_t instructionIndex = 0; instructionIndex < instructionBuffer.size(); ++instructionIndex) {
		if (is16bit) {
			instructionBuffer[instructionIndex].bitdata8 = 0;
			instructionBuffer[instructionIndex].bitdata16 = instructionWords[instructionIndex];
		}
		else {
			instructionBuffer[instructionIndex].bitdata8 = static_cast<uint8_t>(instructionWords[instructionIndex]);
			instructionBuffer[instructionIndex].bitdata16 = 0;
		}
	}
}

//============================================
//			DATA AND FILE OUTPUT
//============================================

void AppendData() {
	if (is16bit) {
		vector<uint16_t> dataWords(dataMap.size(), 0);

		for (const auto& [symbolName, data] : dataMap) {
			dataWords.at(static_cast<size_t>(data.RelativeLocation)) = dataWordValues.at(symbolName);
		}
		final16b.insert(final16b.end(), dataWords.begin(), dataWords.end());
	}
	else {
		vector<uint8_t> dataWords(dataMap.size(), 0);

		for (const auto& [symbolName, data] : dataMap) {
			dataWords.at(static_cast<size_t>(data.RelativeLocation)) = static_cast<uint8_t>(dataWordValues.at(symbolName));
		}
		final8b.insert(final8b.end(), dataWords.begin(), dataWords.end());
	}
}

void WriteByte(ofstream& output, uint8_t byteValue) {
	output.write(reinterpret_cast<const char*>(&byteValue), 1);
}

void WriteBinary(const string& outputFile) {
	ofstream output(outputFile, ios::binary | ios::trunc);

	if (!output) {
		ERROR("Cannot open output file: " + outputFile);
	}
	if (is16bit) {
		for (uint16_t outputWord : final16b) {
			WriteByte(output, static_cast<uint8_t>(outputWord));
			WriteByte(output, static_cast<uint8_t>(outputWord >> 8));
		}
	}
	else {
		for (uint8_t outputWord : final8b) {
			WriteByte(output, outputWord);
		}
	}
	output.close();

	if (!output) {
		ERROR("Failed writing output file: " + outputFile);
	}
}

//============================================
//			ASSEMBLER
//============================================

void assemble(const string& inputFile, const string& outputFile) {
	if (inputFile == outputFile) {
		ERROR("Input and output paths must differ");
	}
	ResetAssembly();
	ProsessFile(inputFile);

	if (rawCode.empty()) {
		ERROR("Input is empty");
	}


	string mode = ToUpper(rawCode.front());

	if (mode == ".16BC") {
		is16bit = true;
	}
	else if (mode != ".8BC") {
		ERROR("First line must be .8bc or .16bc");
	}
	rawCode.erase(rawCode.begin());
	SplitSegments();
	MapData();
	GenerateInstructions();
	MapLabels();
	ResolveSymbols();
	FinalConvert();
	AppendData();
	WriteBinary(outputFile);
}
