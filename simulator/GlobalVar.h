#ifndef GlobalVar_h
#define GlobalVar_h

#include <map>
#include <vector>
#include <string>

using namespace std;

class Instruction {
public:
int Word, opcode, rs,rt, rd, shamt, funct;
short C;
char type;
bool fwdrs, fwdrt;
bool fwd_EX_DM_from;			//false = from EX/DM ; true = from DM/WB
string Name;
Instruction(){
								Word = opcode = rs = rt = rd = shamt = funct = 0;
								C = 0;
								type = '\0';
								fwd_EX_DM_from = fwdrs = fwdrt = false;
								Name = "NOP";
}
};

class Buffer {
public:
Instruction ins;
int ALU_result, Data, RegRs, RegRt, WriteDes;
bool RegWrite, MemRead;
Buffer(){
								RegRs = RegRt = Data = ALU_result = WriteDes = 0;
								RegWrite = MemRead = false;
}
void Clear(){
								ALU_result = 0;
								Data = 0;
								RegRs = 0;
								RegRt = 0;
								WriteDes = 0;
								RegWrite = false;
								MemRead = false;
}
};

class lastThings{
public:
	static int reg[32],LO,HI;
};


class Global {
public:
static int Address[1024];
static map< int,char > Memory;
static int reg[32], PC, Branch_PC,LO,HI;
static bool Halt, Stall;
static bool Branch_taken;
static bool error_type[5];
static bool toggle_MULT,toggle_HILO;
static Buffer IF_ID, ID_EX, EX_MEM, MEM_WB;
static bool Flush;
};
#endif
