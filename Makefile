CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
COMPILER_SOURCES = Xbc\ Compiler/src/main.cpp Xbc\ Compiler/src/asembler/assembler.cpp Xbc\ Compiler/src/asembler/dataHandler.cpp Xbc\ Compiler/src/CoreHelp.cpp
COMPILER_HEADERS = Xbc\ Compiler/src/asembler/asm.h Xbc\ Compiler/src/asembler/datahandler.h Xbc\ Compiler/src/asembler/instructionset.h Xbc\ Compiler/src/CoreHelp.h common/float16.h
all: xbc xbc-sim
xbc: $(COMPILER_SOURCES) $(COMPILER_HEADERS)
	$(CXX) $(CXXFLAGS) 'Xbc Compiler/src/main.cpp' 'Xbc Compiler/src/asembler/assembler.cpp' 'Xbc Compiler/src/asembler/dataHandler.cpp' 'Xbc Compiler/src/CoreHelp.cpp' -o $@
xbc-sim: xbcSim/src/main.cpp common/float16.h
	$(CXX) $(CXXFLAGS) xbcSim/src/main.cpp -o $@
test: all
	python3 tests/test.py
clean:
	rm -f xbc xbc-sim
.PHONY: all test clean
