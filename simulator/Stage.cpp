#include <iostream>
#include <bitset>
#include "Simulator.h"
#include "Stage.h"

using namespace std;

inline void NumberOverflowDetect(int result, int A, int B) {
	bitset<32> bs1 = result, bs2 = A, bs3 = B;
	if (bs2[31] == bs3[31] && bs2[31] != bs1[31])
		Global::error_type[3] = true;
}

inline void HILOWriteDetect(string op) {
	if (op == "mult" || op == "multu")
	{
		if (Global::toggle_MULT == 1 && Global::toggle_HILO == 1) //NO HILO error , turn off HILO toggle now
		{
			Global::toggle_HILO = 0;
		}
		else if (Global::toggle_MULT == 1 && Global::toggle_HILO == 0) //HILO error!!
		{
			Global::error_type[4] = 1;
		}
		//NEED TO DETECT HILO ON AND TURN IT OFF BEFORE FIRST MULT

		else {
			Global::toggle_MULT = 1; Global::toggle_HILO = 0;
		}
	}
	else //op == hi lo
	{
		Global::toggle_HILO = 1;
	}
}

inline void AddressOverflowDetect(int a, int bytes) {
	if (a < 0 || a >= 1024 || a + bytes >= 1024 || a + bytes < 0)
		Global::error_type[1] = true;
}

inline void DataMisaligned(int a, int bytes) {
	if (a % bytes != 0)
		Global::error_type[2] = true;
}

inline bool isBranch(Instruction ins) {
	return ((ins.Name == "BEQ") || (ins.Name == "BNE") || (ins.Name == "BGTZ") || (ins.Name == "J") || (ins.Name == "JAL") || (ins.Name == "JR"));
}

inline bool notS(string s) {
	return (s == "NOP") || (s == "SW") || (s == "SB") || (s == "SH");
}
inline bool isHILO(Instruction ins) {
	return ((ins.Name == "MULT") || (ins.Name == "MULTU") || (ins.Name == "MFHI") || (ins.Name == "MFLO"));
}

void IF() {
	Instruction ins;
	ins.line = Global::Address[Global::PC];
	// Calculate opcode
	ins.opcode = ((unsigned int)ins.line) >> 26;
	// Calculate rs
	ins.rs = ((unsigned int)(ins.line << 6)) >> 27;
	// Calculate rt
	ins.rt = ((unsigned int)(ins.line << 11)) >> 27;
	// Calculate rd
	ins.rd = ((unsigned int)(ins.line << 16)) >> 27;
	// Calculate shamt
	ins.shamt = ((unsigned int)(ins.line << 21)) >> 27;
	// Calculate funct
	ins.funct = ((unsigned int)(ins.line << 26)) >> 26;
	// Calculate imme
	ins.imme = (ins.line << 16) >> 16;
	switch (ins.opcode) {
	case 0:
		ins.type = 'R';
		switch (ins.funct) {
		case 32: // add
			ins.Name = "ADD";
			break;
		case 33: // addu
			ins.Name = "ADDU";
			break;
		case 34: // sub
			ins.Name = "SUB";
			break;
		case 36: // and
			ins.Name = "AND";
			break;
		case 37: // or
			ins.Name = "OR";
			break;
		case 38: // xor
			ins.Name = "XOR";
			break;
		case 39: // nor
			ins.Name = "NOR";
			break;
		case 40: // nand
			ins.Name = "NAND";
			break;
		case 42: // slt
			ins.Name = "SLT";
			break;
		case 0: // sll
			ins.Name = "SLL";
			break;
		case 2: // srl
			ins.Name = "SRL";
			break;
		case 3: // sra
			ins.Name = "SRA";
			break;
		case 8: // jr
			ins.Name = "JR";
			break;
		case 24:// mult
			ins.Name = "MULT";
			break;
		case 25:// multu
			ins.Name = "MULTU";
			break;
		case 16:// mfhi
			ins.Name = "MFHI";
			break;
		case 18:// mflo
			ins.Name = "MFLO";
			break;
		}
		break;
	case 8: // addi
		ins.type = 'I';
		ins.Name = "ADDI";
		break;
	case 9: // addiu
		ins.type = 'I';
		ins.Name = "ADDIU";
		break;
	case 35: // lw
		ins.type = 'I';
		ins.Name = "LW";
		break;
	case 33: // lh
		ins.type = 'I';
		ins.Name = "LH";
		break;
	case 37: // lhu
		ins.type = 'I';
		ins.Name = "LHU";
		break;
	case 32: // lb
		ins.type = 'I';
		ins.Name = "LB";
		break;
	case 36: // lbu
		ins.type = 'I';
		ins.Name = "LBU";
		break;
	case 43: // sw
		ins.type = 'I';
		ins.Name = "SW";
		break;
	case 41: // sh
		ins.type = 'I';
		ins.Name = "SH";
		break;
	case 40: // sb
		ins.type = 'I';
		ins.Name = "SB";
		break;
	case 15: // lui
		ins.type = 'I';
		ins.Name = "LUI";
		break;
	case 12: // andi
		ins.type = 'I';
		ins.Name = "ANDI";
		break;
	case 13: // ori
		ins.type = 'I';
		ins.Name = "ORI";
		break;
	case 14: // nori
		ins.type = 'I';
		ins.Name = "NORI";
		break;
	case 10: // slti
		ins.type = 'I';
		ins.Name = "SLTI";
		break;
	case 63: // Halt
		ins.Name = "HALT";
		break;
	case 4: // beq
		ins.Name = "BEQ";
		break;
	case 5: // bne
		ins.Name = "BNE";
		break;
	case 7: // bgtz
		ins.Name = "BGTZ";
		break;
	case 2: // j
		ins.Name = "J";
		break;
	case 3: // jal
		ins.type = 'J';
		ins.Name = "JAL";
		break;
	}
	if (ins.Name == "SLL")
		if (ins.rt == 0 && ins.rd == 0 && ins.shamt == 0)
			ins.Name = "NOP";
	if (!Global::Stall) Global::IF_ID.ins = ins;
}

void ID() {
	Instruction ins = Global::IF_ID.ins;
	int jumpTaregt = 0, DataRs, DataRt;
	Global::ID_EX.RegWrite = false;
	Global::ID_EX.MemRead = false;

	if (Global::Stall) {
		Instruction NOP;
		Global::ID_EX.ins = NOP;
		Global::ID_EX.Clear();
		return;
	}

	// Forwarding
	if (ins.fwdrs)
		DataRs = Global::MEM_WB.ALU_result;
	else
		DataRs = Global::reg[ins.rs];
	if (ins.fwdrt)
		DataRt = Global::MEM_WB.ALU_result;
	else
		DataRt = Global::reg[ins.rt];

	//Do ID things
	switch (ins.opcode) {
	case 0:
		switch (ins.funct) {
		case 32: // add
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 33: // addu
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 34: // sub
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 36: // and
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 37: // or
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 38: // xor
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 39: // nor
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 40: // nand
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 42: // slt
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 0: // sll
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 2: // srl
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 3: // sra
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 8: // jr
			Global::Branch_taken = true;
			Global::Branch_PC = DataRs;
			break;
		case 24: // mult
			Global::ID_EX.RegWrite = false;
			break;
		case 25: // multu
			Global::ID_EX.RegWrite = false;
			break;
		case 16: // mfhi
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		case 18: // mflo
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = ins.rd;
			break;
		}
		break;
	case 8: // addi
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 9: // addiu
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 35: // lw
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 33: // lh
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 37: // lhu
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 32: // lb
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 36: // lbu
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 15: // lui
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 12: // andi
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 13: // ori
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 14: // nori
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 10: // slti
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = ins.rt;
		break;
	case 63: // Halt
		Global::Halt = true;
		break;
	case 4: // beq
		Global::Branch_taken = (DataRs == DataRt);
		NumberOverflowDetect(Global::PC + 4, Global::PC, 4);
		NumberOverflowDetect(Global::PC + 4 + 4 * (int)ins.imme, Global::PC + 4, 4 * (int)ins.imme);
		Global::Branch_PC = Global::PC + 4 * (int)ins.imme;
		break;
	case 5: // bne
		Global::Branch_taken = (DataRs != DataRt);
		NumberOverflowDetect(Global::PC + 4, Global::PC, 4);
		NumberOverflowDetect(Global::PC + 4 + 4 * (int)ins.imme, Global::PC + 4, 4 * (int)ins.imme);
		Global::Branch_PC = Global::PC + 4 * (int)ins.imme;
		break;
	case 7: // bgtz
		Global::Branch_taken = (DataRs > 0);
		NumberOverflowDetect(Global::PC + 4, Global::PC, 4);
		NumberOverflowDetect(Global::PC + 4 + 4 * (int)ins.imme, Global::PC + 4, 4 * (int)ins.imme);
		Global::Branch_PC = Global::PC + 4 * (int)ins.imme;
		break;
	case 2: // j
		ins.imme = (unsigned)(ins.line << 6) >> 6;
		jumpTaregt = ((Global::PC + 4) >> 28) << 28;
		jumpTaregt += (ins.imme << 2);
		Global::Branch_taken = true;
		Global::Branch_PC = jumpTaregt;
		break;
	case 3: // jal
		Global::ID_EX.WriteDes = 31;
		Global::ID_EX.ALU_result = Global::PC;
		ins.imme = (unsigned)(ins.line << 6) >> 6;
		jumpTaregt = ((Global::PC + 4) >> 28) << 28;
		jumpTaregt += (ins.imme << 2);
		Global::ID_EX.RegWrite = true;
		Global::Branch_taken = true;
		Global::Branch_PC = jumpTaregt;
		break;
	}
	if (!Global::Stall) Global::ID_EX.ins = ins;
	Global::ID_EX.RegRs = DataRs;
	Global::ID_EX.RegRt = DataRt;
}

void EXE() {
	Instruction ins = Global::ID_EX.ins;
	int ALU_result = 0, DataRs = 0, DataRt = 0;
	// Forwarding
	if (!isBranch(ins)) {
		if (ins.fwdrs)
		{
			DataRs = Global::MEM_WB.Data;
			if (ins.fwdrs_EX_DM_from)
				DataRs = Global::WB_AFTER.ALU_result;
		}
		else
			DataRs = Global::ID_EX.RegRs;
		if (ins.fwdrt)
		{
			DataRt = Global::MEM_WB.Data;
			if (ins.fwdrt_EX_DM_from)
				DataRt = Global::WB_AFTER.ALU_result;
		}
		else
			DataRt = Global::ID_EX.RegRt;
	}

	ins.fwdrs = false;
	ins.fwdrt = false;
	ins.fwdrt_EX_DM_from = false;
	ins.fwdrs_EX_DM_from = false;
	unsigned int A;
	unsigned long long x = 0;
	unsigned long long rs_64, rt_64;
	switch (ins.opcode) {
	case 0:
		switch (ins.funct) {
		case 32: // add
			ALU_result = DataRs + DataRt;
			NumberOverflowDetect(ALU_result, DataRs, DataRt);
			break;
		case 33: // addu
			ALU_result = DataRs + DataRt;
			break;
		case 34: // sub
			ALU_result = DataRs - DataRt;
			NumberOverflowDetect(ALU_result, DataRs, (~DataRt) + 1);
			break;
		case 36: // and
			ALU_result = DataRs & DataRt;
			break;
		case 37: // or
			ALU_result = DataRs | DataRt;
			break;
		case 38: // xor
			ALU_result = DataRs ^ DataRt;
			break;
		case 39: // nor
			ALU_result = ~(DataRs | DataRt);
			break;
		case 40: // nand
			ALU_result = ~(DataRs & DataRt);
			break;
		case 42: // slt
			ALU_result = (DataRs < DataRt);
			break;
		case 0: // sll
			ALU_result = DataRt << ins.shamt;
			break;
		case 2: // srl
			ALU_result = (unsigned int)DataRt >> ins.shamt;
			break;
		case 3: // sra
			ALU_result = DataRt >> ins.shamt;
			break;
		case 24:// mult
			ALU_result = Global::ID_EX.ALU_result;
			HILOWriteDetect("mult");
			x = (long long)DataRs*(long long)DataRt;
			Global::HI = x >> 32;
			Global::LO = x << 32 >> 32;
			break;
		case 25:// multu
			ALU_result = Global::ID_EX.ALU_result;
			HILOWriteDetect("multu");
			rs_64 = (unsigned)DataRs;
			rt_64 = (unsigned)DataRt;
			x = rs_64*rt_64;
			Global::HI = x >> 32;
			Global::LO = x << 32 >> 32;
			break;
		case 16:// mfhi
			ALU_result = Global::HI;
			HILOWriteDetect("mfhi");
			break;
		case 18:// mflo
			ALU_result = Global::LO;
			HILOWriteDetect("mflo");
			break;
		}
		break;
	case 8: // addi
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 9: // addiu
		ALU_result = DataRs + (int)ins.imme;
		break;
	case 35: // lw
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 33: // lh
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 37: // lhu
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 32: // lb
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 36: // lbu
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 43: // sw
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 41: // sh
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 40: // sb
		ALU_result = DataRs + (int)ins.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)ins.imme);
		break;
	case 15: // lui
		ALU_result = (int)ins.imme << 16;
		break;
	case 12: // andi
		A = (((unsigned int)ins.imme << 16) >> 16);
		ALU_result = DataRs & A;
		break;
	case 13: // ori
		A = (((unsigned int)ins.imme << 16) >> 16);
		ALU_result = DataRs | A;
		break;
	case 14: // nori
		A = (((unsigned int)ins.imme << 16) >> 16);
		ALU_result = ~(DataRs | A);
		break;
	case 10: // slti
		ALU_result = (DataRs < ins.imme);
		break;
	case 3: //jal
		ALU_result = Global::ID_EX.ALU_result;
		break;
	case 63: // Halt
		Global::Halt = true;
		break;
	}
	//Passing results
	Global::EX_MEM.ALU_result = ALU_result;
	Global::EX_MEM.ins = ins;
	Global::EX_MEM.RegRt = DataRt;
	Global::EX_MEM.RegWrite = Global::ID_EX.RegWrite;
	Global::EX_MEM.MemRead = Global::ID_EX.MemRead;
	Global::EX_MEM.WriteDes = Global::ID_EX.WriteDes;
}

void DM() {
	Instruction ins = Global::EX_MEM.ins;
	int Data = 0;

	switch (ins.opcode) {
	case 35: // lw
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 3);
		DataMisaligned(Global::EX_MEM.ALU_result, 4);
		if (Global::error_type[1] || Global::error_type[2]) return;
		for (int i = 0; i < 4; i++)
			Data = (Data << 8) | (unsigned char)Global::Memory[Global::EX_MEM.ALU_result + i];
		break;
	case 33: // lh
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 1);
		DataMisaligned(Global::EX_MEM.ALU_result, 2);
		if (Global::error_type[1] || Global::error_type[2]) return;
		Data = Global::Memory[Global::EX_MEM.ALU_result];
		for (int i = 1; i < 2; i++)
			Data = (Data << 8) | (unsigned char)Global::Memory[Global::EX_MEM.ALU_result + i];
		break;
	case 37: // lhu
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 1);
		DataMisaligned(Global::EX_MEM.ALU_result, 2);
		if (Global::error_type[1] || Global::error_type[2]) return;
		for (int i = 0; i < 2; i++)
			Data = (Data << 8) | (unsigned char)Global::Memory[Global::EX_MEM.ALU_result + i];
		break;
	case 32: // lb
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 0);
		if (Global::error_type[1]) return;
		Data = Global::Memory[Global::EX_MEM.ALU_result];
		break;
	case 36: // lbu
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 0);
		if (Global::error_type[1]) return;
		Data = (unsigned char)Global::Memory[Global::EX_MEM.ALU_result];
		break;
	case 43: // sw
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 3);
		DataMisaligned(Global::EX_MEM.ALU_result, 4);
		if (Global::error_type[1] || Global::error_type[2]) return;
		for (int i = 0; i < 4; i++)
			Global::Memory[Global::EX_MEM.ALU_result + i] = (char)(Global::EX_MEM.RegRt >> (8 * (3 - i)));
		break;
	case 41: // sh
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 1);
		DataMisaligned(Global::EX_MEM.ALU_result, 2);
		if (Global::error_type[1] || Global::error_type[2]) return;
		for (int i = 0; i < 2; i++)
			Global::Memory[Global::EX_MEM.ALU_result + i] = (char)(Global::EX_MEM.RegRt >> (8 * (1 - i)));
		break;
	case 40: // sb
		AddressOverflowDetect(Global::EX_MEM.ALU_result, 0);
		if (Global::error_type[1]) return;
		Global::Memory[Global::EX_MEM.ALU_result] = (char)Global::EX_MEM.RegRt;
		break;
	default: // Store Load else
		Data = Global::EX_MEM.ALU_result;
		break;
	}
	Global::MEM_WB.Data = Data;
	Global::MEM_WB.ALU_result = Global::EX_MEM.ALU_result;
	Global::MEM_WB.ins = ins;
	Global::MEM_WB.RegRt = Global::EX_MEM.RegRt;
	Global::MEM_WB.RegWrite = Global::EX_MEM.RegWrite;
	Global::MEM_WB.MemRead = Global::EX_MEM.MemRead;
	Global::MEM_WB.WriteDes = Global::EX_MEM.WriteDes;
	//Global::debug();
}

void WB() {
	Instruction ins = Global::MEM_WB.ins;
	if (Global::MEM_WB.RegWrite) {
		if ((Global::MEM_WB.WriteDes == 0) && !isBranch(ins) && !notS(ins.Name)) {
			Global::error_type[0] = true;		//Write 0
			return;
		}
		if (ins.type == 'R' || ins.type == 'J')
		{
			Global::reg[Global::MEM_WB.WriteDes] = Global::MEM_WB.ALU_result;
			Global::WB_AFTER.ALU_result = Global::MEM_WB.ALU_result;
		}
		else if (ins.type == 'I')
		{
			Global::reg[Global::MEM_WB.WriteDes] = Global::MEM_WB.Data;
			Global::WB_AFTER.ALU_result = Global::MEM_WB.Data;
		}
	}
}
