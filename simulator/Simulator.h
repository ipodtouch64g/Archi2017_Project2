#ifndef Simulator_h
#define Simulator_h


#include <string>
#include <iostream>
using namespace std;

class Instruction {
public:
	int line, opcode, rs, rt, rd, shamt, funct;
	short imme;
	char type;
	bool fwdrs, fwdrt;
	bool fwdrs_EX_DM_from, fwdrt_EX_DM_from;			//false = from EX/DM ; true = from DM/WB
	string Name;
	Instruction() {
		line = opcode = rs = rt = rd = shamt = funct = 0;
		imme = 0;
		type = '\0';
		fwdrt_EX_DM_from = fwdrs_EX_DM_from = fwdrs = fwdrt = false;
		Name = "NOP";
	}
};

class Buffer {
public:
	Instruction ins;
	int ALU_result, Data, RegRs, RegRt, WriteDes;
	bool RegWrite, MemRead;
	Buffer() {
		RegRs = RegRt = Data = ALU_result = WriteDes = 0;
		RegWrite = MemRead = false;
	}
	void Clear() {
		ALU_result = 0;
		Data = 0;
		RegRs = 0;
		RegRt = 0;
		WriteDes = 0;
		RegWrite = false;
		MemRead = false;
	}
};

class lastThings {
public:
	static int reg[32], LO, HI;
};


class Global {
public:
	static int Address[1024];
	static int Memory[1024];
	static bool Branch_taken;
	static bool error_type[5];
	static void debug();
	static bool Halt, Stall; static Buffer IF_ID, ID_EX, EX_MEM, MEM_WB, WB_AFTER;
	static bool Flush;
	static int reg[32], PC, Branch_PC, LO, HI;
	static bool toggle_MULT, toggle_HILO;
};
#endif
