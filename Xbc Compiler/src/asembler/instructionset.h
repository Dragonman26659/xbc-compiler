#pragma once
#include <unordered_map>


enum INSTRUCTIONS {
	// 8bc
	LDA_IMM,
	ADD,
	SUB,
	XOR,
	OR,
	AND,
	NOT,
	BRA,
	BRZ,
	BRN,
	BRP,
	STA,
	STR,
	RRAM,
	WRAM,
	LSHIFT,
	RSHIFT,
	POP,
	PUSH,
	SETINTR,
	HLT8,

	//16bc
	MULT,
	DIV,
	FLADD,
	FLSUB,
	FLMULT,
	FLDIV,
	SMULT,
	SDIV,
	FCMP,
	ITOF,				// Signed int to float
	UTOF,				// Unsigned int to float
	FTOI,				// Float to signed int
	FTOU,				// Float to unsigned int
	HLT16
};