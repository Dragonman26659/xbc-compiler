#include "asembler/asm.h"

#include <iostream>
#include <string>
#include <exception>


int main(int argc, char* argv[]) {

    if (argc != 4) {
        std::cerr << "Usage: xbc <input file> <asm|c> <output file>\n";
        return 1;
    }


    std::string inputFile = argv[1];
    std::string inputType = argv[2];
    std::string outputFile = argv[3];


    try {

        if (inputType == "asm") {
            assemble(inputFile, outputFile);
        }
        else if (inputType == "c") {
            std::cerr << "C compilation is not implemented yet.\n";
            return 1;
        }
        else {
            std::cerr << "Unknown input type: " << inputType << "\n";
            return 1;
        }

    }
    catch (const std::exception& error) {
        std::cerr << "Build failed: " << error.what() << "\n";
        return 1;
    }


    return 0;
}