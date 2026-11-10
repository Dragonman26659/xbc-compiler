#include <cstdint>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <deque>
#include <array>
#include <algorithm>

//============================================
//              FLOAT CONVERSION
//============================================

#include "../../common/float16.h"
using xbc::DecodeFloat16;
using xbc::EncodeFloat16;

//============================================
//              SIMULATOR STATE
//============================================

class Simulator {
private:
    unsigned architectureBits;
    uint32_t wordMask;

    std::vector<uint16_t> memory;
    std::vector<uint16_t> registers;
    size_t stackBottom, stackEnd, idtBase;
    size_t executingPC = 0;
    uint64_t timerTicks = 0;
    struct Event { unsigned vector; std::array<uint16_t, 3> values; };
    std::deque<Event> pendingEvents;
    struct Fault { unsigned vector; };

    unsigned SegfaultVector() const { return architectureBits == 8 ? 1 : 3; }
    unsigned OverflowVector() const { return architectureBits == 8 ? 2 : 4; }
    unsigned UnderflowVector() const { return architectureBits == 8 ? 3 : 5; }
    void ValidateAddress(size_t address) const {
        if (address >= memory.size() || (address >= stackBottom && address < stackEnd))
            throw Fault{SegfaultVector()};
    }
    void ValidateSP() const {
        if (registers[15] < stackBottom || registers[15] > stackEnd)
            throw Fault{SegfaultVector()};
    }
    void PushWord(uint16_t value) {
        ValidateSP();
        if (registers[15] == stackBottom) throw Fault{OverflowVector()};
        memory[--registers[15]] = value & wordMask;
    }
    uint16_t PopWord() {
        ValidateSP();
        if (registers[15] == stackEnd) throw Fault{UnderflowVector()};
        return memory[registers[15]++];
    }
    void Dispatch(unsigned vector, bool error, const std::array<uint16_t, 3>& values) {
        // Validate the entire frame before writing any part of it.
        size_t frameSize = error ? 2 : (architectureBits == 16 ? 15 : 1);
        if (registers[15] < stackBottom || registers[15] > stackEnd ||
            size_t(registers[15] - stackBottom) < frameSize)
            throw std::runtime_error("Interrupt frame cannot fit on stack");
        size_t target = memory[idtBase + vector];
        try { ValidateAddress(target); }
        catch (const Fault&) { throw std::runtime_error("Invalid IDT handler address"); }
        uint16_t oldEAX = registers[0];
        PushWord(static_cast<uint16_t>(error ? executingPC : programCounter));
        if (error) { PushWord(oldEAX); registers[0] = static_cast<uint16_t>(vector); }
        else if (architectureBits == 16) {
            // Save R1-R14 only. R15 is volatile; SP is restored by popping.
            for (int i = 13; i >= 0; --i) PushWord(registers[i]);
            if (vector == 0 || vector == 1) registers[0] = values[0];
            if (vector == 1) { registers[1] = values[1]; registers[2] = values[2]; }
        }
        else { registers[0] = values[0] & wordMask; }
        interruptsEnabled = false;
        programCounter = target;
    }
    void EndCycle() {
        if (architectureBits == 16) {
            uint16_t period = memory[65535];
            if (period == 0) timerTicks = 0;
            else if (++timerTicks >= period) {
                timerTicks = 0;
                bool alreadyPending = std::any_of(pendingEvents.begin(), pendingEvents.end(),
                    [](const Event& e) { return e.vector == 2; });
                if (!alreadyPending) pendingEvents.push_back(Event{2, {0, 0, 0}});
            }
        }
        if (interruptsEnabled && !pendingEvents.empty()) {
            Event event = pendingEvents.front();
            Dispatch(event.vector, false, event.values);
            pendingEvents.pop_front();
        }
    }

    uint16_t accumulator = 0;
    size_t programCounter = 0;
    uint64_t instructionCount = 0;

    bool zeroFlag = false;
    bool negativeFlag = false;
    bool unorderedFlag = false;
    bool interruptsEnabled = false;
    bool halted = false;

    // Prevent a mistaken endless loop from running forever.
    const uint64_t maximumInstructions = 10000000;

    int32_t SignedValue(uint16_t value) const {
        uint32_t signBit = (wordMask + 1) >> 1;
        int32_t result = static_cast<int32_t>(value & wordMask);

        if ((value & signBit) != 0) {
            result -= static_cast<int32_t>(wordMask + 1);
        }

        return result;
    }

    uint16_t WrapInteger(int64_t value) const {
        return static_cast<uint16_t>(static_cast<uint64_t>(value) & wordMask);
    }

    void SetIntegerResult(int64_t value) {
        accumulator = WrapInteger(value);

        zeroFlag = accumulator == 0;
        negativeFlag = SignedValue(accumulator) < 0;
        unorderedFlag = false;
    }

    void SetFloatResult(double value) {
        accumulator = EncodeFloat16(value);
        double roundedValue = DecodeFloat16(accumulator);

        zeroFlag = roundedValue == 0;
        negativeFlag = roundedValue < 0;
        unorderedFlag = std::isnan(roundedValue);
    }

    uint16_t ReadNextWord() {
        ValidateAddress(programCounter);
        uint16_t word = memory[programCounter];
        ++programCounter;

        return word;
    }

    void InvalidInstruction(uint16_t instruction) const {
        throw std::runtime_error("Invalid instruction " +
            std::to_string(instruction));
    }


    //============================================
    //              16BC EXTENSIONS
    //============================================

    void ExecuteExtension(uint16_t instruction) {
        unsigned opcode = (instruction >> 8) & 0x0F;
        unsigned registerA = (instruction >> 4) & 0x0F;
        unsigned registerB = instruction & 0x0F;

        uint16_t valueA = registers[registerA];
        uint16_t valueB = registers[registerB];

        double floatA = DecodeFloat16(valueA);
        double floatB = DecodeFloat16(valueB);

        switch (opcode) {
        case 0x8:
            SetIntegerResult(static_cast<int64_t>(valueA) * valueB);
            break;

        case 0x4:
            if (valueB == 0) {
                throw Fault{6};
            }

            SetIntegerResult(valueA / valueB);
            break;

        case 0xC:
            SetFloatResult(floatA + floatB);
            break;

        case 0x2:
            SetFloatResult(floatA - floatB);
            break;

        case 0xA:
            SetFloatResult(floatA * floatB);
            break;

        case 0x6:
            if (floatB == 0) throw Fault{6};
            SetFloatResult(floatA / floatB);
            break;

        case 0xE:
            SetIntegerResult(static_cast<int64_t>(SignedValue(valueA)) *
                SignedValue(valueB));
            break;

        case 0x1:
            if (valueB == 0) {
                throw Fault{6};
            }

            SetIntegerResult(static_cast<int64_t>(SignedValue(valueA)) /
                SignedValue(valueB));
            break;

        case 0x9:
            // FCMP is an ALU subtraction without a register writeback.
            SetFloatResult(floatA - floatB);
            break;

        case 0x5:
            registers[registerB] = EncodeFloat16(SignedValue(valueA));
            break;

        case 0xD:
            registers[registerB] = EncodeFloat16(valueA);
            break;

        case 0x3:
        case 0xB: {
            double integerValue = std::trunc(floatA);
            double minimumValue = 0;
            double maximumValue = 65535;

            if (opcode == 0x3) {
                minimumValue = -32768;
                maximumValue = 32767;
            }

            if (!std::isfinite(integerValue) || integerValue < minimumValue ||
                integerValue > maximumValue) {
                throw std::runtime_error("Float-to-integer conversion out of range");
            }

            registers[registerB] = WrapInteger(static_cast<int64_t>(integerValue));
            break;
        }

        default:
            InvalidInstruction(instruction);
        }
    }


    //============================================
    //              MEMORY AND STACK
    //============================================

    void ExecuteMemoryInstruction(uint16_t instruction) {
        unsigned registerIndex = instruction & 0x03;
        unsigned opcode = instruction & 0xFC;

        switch (opcode) {
        case 0xF0:
            ValidateAddress(accumulator);
            registers[registerIndex] = memory[accumulator];
            registers[14] = memory[accumulator];
            break;

        case 0xE8:
            ValidateAddress(accumulator);
            memory[accumulator] = registers[registerIndex];
            break;

        case 0xF8:
            SetIntegerResult(static_cast<int64_t>(registers[registerIndex]) << 1);
            registers[registerIndex] = accumulator;
            break;

        case 0xE4:
            // Logical right shift, filling the high bit with zero.
            SetIntegerResult(registers[registerIndex] >> 1);
            registers[registerIndex] = accumulator;
            break;

        case 0xF4:
            registers[registerIndex] = PopWord();
            break;

        case 0xEC:
            PushWord(registers[registerIndex]);
            break;

        case 0xE0:
            if ((instruction & 1) != 0) {
                InvalidInstruction(instruction);
            }

            interruptsEnabled = (instruction & 2) != 0;
            break;

        default:
            InvalidInstruction(instruction);
        }
    }


    //============================================
    //              LEGACY INSTRUCTIONS
    //============================================

    void ExecuteLegacyInstruction(uint16_t instruction) {
        if (instruction < 16) {
            registers[instruction] = ReadNextWord();
            return;
        }

        if ((instruction & 0xE0) == 0xE0) {
            ExecuteMemoryInstruction(instruction);
            return;
        }

        unsigned opcode = instruction & 0xE1;
        unsigned registerA = (instruction >> 3) & 0x03;
        unsigned registerB = (instruction >> 1) & 0x03;
        unsigned singleRegister = (instruction >> 1) & 0x0F;

        switch (opcode) {
        case 0x80:
            SetIntegerResult(registers[registerA] + registers[registerB]);
            break;

        case 0x40:
            SetIntegerResult(static_cast<int32_t>(registers[registerA]) -
                registers[registerB]);
            break;

        case 0xC0:
            SetIntegerResult(registers[registerA] ^ registers[registerB]);
            break;

        case 0x20:
            SetIntegerResult(registers[registerA] | registers[registerB]);
            break;

        case 0x60:
            SetIntegerResult(registers[registerA] & registers[registerB]);
            break;

        case 0xA0:
            SetIntegerResult(static_cast<int64_t>(~registers[singleRegister]));
            break;

        case 0x81:
            programCounter = registers[singleRegister];
            break;

        case 0x41:
            if (zeroFlag) {
                programCounter = registers[singleRegister];
            }
            break;

        case 0xC1:
            if (negativeFlag) {
                programCounter = registers[singleRegister];
            }
            break;

        case 0x21:
            if (!zeroFlag && !negativeFlag && !unorderedFlag) {
                programCounter = registers[singleRegister];
            }
            break;

        case 0xA1:
            registers[singleRegister] = accumulator;
            break;

        case 0x61:
            SetIntegerResult(registers[singleRegister]);
            break;

        default:
            InvalidInstruction(instruction);
        }
    }


public:
    explicit Simulator(unsigned bits) {
        if (bits != 8 && bits != 16) {
            throw std::runtime_error("Architecture must be 8 or 16");
        }

        architectureBits = bits;
        wordMask = (1u << bits) - 1;

        memory.resize(static_cast<size_t>(wordMask) + 1, 0);
        registers.resize(16, 0);
        stackBottom = bits == 8 ? 206 : 59180;
        stackEnd = bits == 8 ? 254 : 63276;
        idtBase = bits == 8 ? 200 : 63278;
        registers[15] = static_cast<uint16_t>(stackEnd);
    }


    void LoadBinary(const std::string& fileName) {
        std::ifstream input(fileName, std::ios::binary);

        if (!input) {
            throw std::runtime_error("Cannot open binary file: " + fileName);
        }

        size_t wordAddress = 0;
        char lowByte = 0;

        while (input.get(lowByte)) {
            if (wordAddress >= memory.size()) {
                throw std::runtime_error("Binary file is larger than memory");
            }

            uint16_t word = static_cast<unsigned char>(lowByte);

            if (architectureBits == 16) {
                char highByte = 0;

                if (!input.get(highByte)) {
                    throw std::runtime_error("16-bit binary has an incomplete word");
                }

                word |= static_cast<uint16_t>(
                    static_cast<unsigned char>(highByte) << 8);
            }

            memory[wordAddress] = word;
            ++wordAddress;
        }

        if (input.bad()) {
            throw std::runtime_error("Failed reading binary file");
        }

        if (wordAddress == 0) {
            throw std::runtime_error("Binary file is empty");
        }
    }


    // Host/device API; regular CPU accesses still enforce stack protection.
    void SetMemory(size_t address, uint16_t value) { memory.at(address) = value & wordMask; }
    uint16_t GetMemory(size_t address) const { return memory.at(address); }
    uint16_t GetRegister(unsigned index) const { return registers.at(index); }
    void SetRegister(unsigned index, uint16_t value) { registers.at(index) = value & wordMask; }
    uint16_t GetAccumulator() const { return accumulator; }
    size_t GetPC() const { return programCounter; }
    bool IsHalted() const { return halted; }
    bool ZeroFlag() const { return zeroFlag; }
    bool NegativeFlag() const { return negativeFlag; }
    bool UnorderedFlag() const { return unorderedFlag; }
    bool InterruptsEnabled() const { return interruptsEnabled; }
    void QueueNumber(uint16_t number) {
        if (architectureBits != 8) throw std::runtime_error("Number input requires 8BC");
        pendingEvents.push_back(Event{0, {uint16_t(number & wordMask), 0, 0}});
    }
    void QueueKeyboard(uint16_t keycode) {
        if (architectureBits != 16) throw std::runtime_error("Keyboard requires 16BC");
        pendingEvents.push_back(Event{0, {keycode, 0, 0}});
    }
    void QueueMouse(uint16_t buttons, int16_t dx, int16_t dy) {
        if (architectureBits != 16) throw std::runtime_error("Mouse requires 16BC");
        pendingEvents.push_back(Event{1, {buttons, uint16_t(dx), uint16_t(dy)}});
    }
    void Step() {
        if (halted) return;
        if (instructionCount >= maximumInstructions)
            throw std::runtime_error("Instruction limit reached (10000000)");
        executingPC = programCounter;
        ++instructionCount;
        try {
            ValidateAddress(programCounter);
            uint16_t instruction = memory[programCounter];
            if (instruction == wordMask) { halted = true; return; }
            ++programCounter;
            if (architectureBits == 16 && (instruction & 0xF000) == 0xF000)
                ExecuteExtension(instruction);
            else ExecuteLegacyInstruction(instruction & 0xFF);
        }
        catch (const Fault& fault) {
            // Faults are synchronous; SETIRR masks device interrupts only.
            Dispatch(fault.vector, true, {0, 0, 0});
            return;
        }
        EndCycle();
    }
    void Run() { while (!halted) Step(); }

    void DumpRegisters() const {
        std::cout << "\nArchitecture: " << architectureBits << " bit\n";
        std::cout << "Halted: " << halted << "\n";
        std::cout << "PC: " << programCounter << "\n";
        std::cout << "Instructions executed: " << instructionCount << "\n";
        std::cout << "Flags: Z=" << zeroFlag << " N=" << negativeFlag
            << " U=" << unorderedFlag << "\n";
        std::cout << "Interrupts enabled: " << interruptsEnabled << "\n";
        std::cout << "Stack depth: " << (stackEnd - registers[15]) << "\n\n";

        std::cout << std::left << std::setw(10) << "Register"
            << std::setw(10) << "Hex"
            << std::setw(12) << "Unsigned"
            << std::setw(12) << "Signed";

        if (architectureBits == 16) {
            std::cout << "Float16";
        }

        std::cout << "\n";

        const std::vector<std::string> names = {
            "EAX", "EBX", "ECX", "EDX",
            "r5", "r6", "r7", "r8", "r9", "r10",
            "r11", "r12", "r13", "r14", "r15", "r16", "ACC"
        };

        for (size_t registerIndex = 0; registerIndex < names.size(); ++registerIndex) {
            uint16_t value = accumulator;

            if (registerIndex < registers.size()) {
                value = registers[registerIndex];
            }

            std::cout << std::left << std::setw(10) << names[registerIndex];
            std::cout << "0x" << std::right << std::setfill('0')
                << std::setw(static_cast<int>(architectureBits / 4))
                << std::hex << std::uppercase << value;
            std::cout << std::dec << std::nouppercase << std::setfill(' ')
                << std::string(8 - architectureBits / 4, ' ');
            std::cout << std::left << std::setw(12) << value
                << std::setw(12) << SignedValue(value);

            if (architectureBits == 16) {
                std::cout << DecodeFloat16(value);
            }

            std::cout << "\n";
        }
    }
};


//============================================
//              MAIN
//============================================

#ifndef XBC_SIM_NO_MAIN
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: xbc-sim <binary file> <8|16> [--number N | --key N | --mouse buttons dx dy]...\n";
        return 1;
    }

    std::string fileName = argv[1];
    std::string architecture = argv[2];
    unsigned bits = 0;

    if (architecture == "8") {
        bits = 8;
    }
    else if (architecture == "16") {
        bits = 16;
    }
    else {
        std::cerr << "Architecture must be 8 or 16.\n";
        return 1;
    }

    Simulator simulator(bits);

    try {
        simulator.LoadBinary(fileName);
        auto parseWord = [](const std::string& token, bool signedInput = false) -> uint16_t {
            size_t consumed = 0;
            long long value = std::stoll(token, &consumed, 0);
            if (consumed != token.size() || value < (signedInput ? -32768 : 0) ||
                value > (signedInput ? 32767 : 65535))
                throw std::runtime_error("Input value out of range: " + token);
            return static_cast<uint16_t>(value);
        };
        for (int i = 3; i < argc; ++i) {
            std::string option = argv[i];
            if ((option == "--number" || option == "--key") && i + 1 < argc) {
                uint16_t value = parseWord(argv[++i]);
                if (option == "--number") {
                    if (value > 255) throw std::runtime_error("Number input exceeds 8 bits");
                    simulator.QueueNumber(value);
                } else simulator.QueueKeyboard(value);
            }
            else if (option == "--mouse" && i + 3 < argc) {
                uint16_t buttons = parseWord(argv[++i]);
                int16_t dx = static_cast<int16_t>(parseWord(argv[++i], true));
                int16_t dy = static_cast<int16_t>(parseWord(argv[++i], true));
                simulator.QueueMouse(buttons, dx, dy);
            }
            else throw std::runtime_error("Invalid event option: " + option);
        }
        simulator.Run();
    }
    catch (const std::exception& error) {
        std::cerr << "Simulation failed: " << error.what() << "\n";
        simulator.DumpRegisters();
        return 1;
    }

    simulator.DumpRegisters();
    return 0;
}

#endif
