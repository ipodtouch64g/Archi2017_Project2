#include <iostream>
#include "Simulator.h"
#include "pipeline.h"

using namespace std;
inline void decodeLine(Instruction &inst)
{
	inst.line = Global::Address[Global::PC];
	inst.opcode = ((unsigned int)inst.line) >> 26;
	inst.rs = ((unsigned int)(inst.line << 6)) >> 27;
	inst.rt = ((unsigned int)(inst.line << 11)) >> 27;
	inst.rd = ((unsigned int)(inst.line << 16)) >> 27;
	inst.shamt = ((unsigned int)(inst.line << 21)) >> 27;
	inst.funct = ((unsigned int)(inst.line << 26)) >> 26;
	inst.imme = (inst.line << 16) >> 16;
}
inline void NumberOverflowDetect(int result, int A, int B) {

	if ((A > 0 && B > 0 && result <= 0) || (A < 0 && B < 0 && result >= 0))
		Global::error_toggle[3] = true;
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
			Global::error_toggle[4] = 1;
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

inline void MemOverflowDetect(int a, int len_byte) {
	if (a < 0 || a >= 1024 || a + len_byte < 0 || a + len_byte >= 1024)
		Global::error_toggle[1] = true;
}

inline void DataMisalignedDetect(int a, int len_byte) {
	if (a % len_byte != 0)
		Global::error_toggle[2] = true;
}

inline bool isBranch(Instruction inst) {
	return ((inst.name == "BEQ") || (inst.name == "BNE") || (inst.name == "BGTZ") || (inst.name == "J") || (inst.name == "JAL") || (inst.name == "JR"));
}

inline bool notS(string s) {
	return (s == "NOP") || (s == "SW") || (s == "SB") || (s == "SH");
}
inline bool isHILO(Instruction inst) {
	return ((inst.name == "MULT") || (inst.name == "MULTU") || (inst.name == "MFHI") || (inst.name == "MFLO"));
}

void IF() {
	Instruction inst;
	decodeLine(inst);
	switch (inst.opcode) {
	case 0:
		inst.type = 'R';
		switch (inst.funct) {
		case 32: // add
			inst.name = "ADD";
			break;
		case 33: // addu
			inst.name = "ADDU";
			break;
		case 34: // sub
			inst.name = "SUB";
			break;
		case 36: // and
			inst.name = "AND";
			break;
		case 37: // or
			inst.name = "OR";
			break;
		case 38: // xor
			inst.name = "XOR";
			break;
		case 39: // nor
			inst.name = "NOR";
			break;
		case 40: // nand
			inst.name = "NAND";
			break;
		case 42: // slt
			inst.name = "SLT";
			break;
		case 0: // sll
			inst.name = "SLL";
			break;
		case 2: // srl
			inst.name = "SRL";
			break;
		case 3: // sra
			inst.name = "SRA";
			break;
		case 8: // jr
			inst.name = "JR";
			break;
		case 24:// mult
			inst.name = "MULT";
			break;
		case 25:// multu
			inst.name = "MULTU";
			break;
		case 16:// mfhi
			inst.name = "MFHI";
			break;
		case 18:// mflo
			inst.name = "MFLO";
			break;
		}
		break;
	case 8: // addi
		inst.type = 'I';
		inst.name = "ADDI";
		break;
	case 9: // addiu
		inst.type = 'I';
		inst.name = "ADDIU";
		break;
	case 35: // lw
		inst.type = 'I';
		inst.name = "LW";
		break;
	case 33: // lh
		inst.type = 'I';
		inst.name = "LH";
		break;
	case 37: // lhu
		inst.type = 'I';
		inst.name = "LHU";
		break;
	case 32: // lb
		inst.type = 'I';
		inst.name = "LB";
		break;
	case 36: // lbu
		inst.type = 'I';
		inst.name = "LBU";
		break;
	case 43: // sw
		inst.type = 'I';
		inst.name = "SW";
		break;
	case 41: // sh
		inst.type = 'I';
		inst.name = "SH";
		break;
	case 40: // sb
		inst.type = 'I';
		inst.name = "SB";
		break;
	case 15: // lui
		inst.type = 'I';
		inst.name = "LUI";
		break;
	case 12: // andi
		inst.type = 'I';
		inst.name = "ANDI";
		break;
	case 13: // ori
		inst.type = 'I';
		inst.name = "ORI";
		break;
	case 14: // nori
		inst.type = 'I';
		inst.name = "NORI";
		break;
	case 10: // slti
		inst.type = 'I';
		inst.name = "SLTI";
		break;
	case 63: // Halt
		inst.name = "HALT";
		break;
	case 4: // beq
		inst.name = "BEQ";
		break;
	case 5: // bne
		inst.name = "BNE";
		break;
	case 7: // bgtz
		inst.name = "BGTZ";
		break;
	case 2: // j
		inst.name = "J";
		break;
	case 3: // jal
		inst.type = 'J';
		inst.name = "JAL";
		break;
	}
	if (inst.name == "SLL")
		if (inst.rt == 0 && inst.rd == 0 && inst.shamt == 0)
			inst.name = "NOP";
	if (!Global::Stall) Global::IF_ID.inst = inst;
}

void ID() {
	Instruction inst = Global::IF_ID.inst;
	int jumpTaregt = 0, DataRs, DataRt;
	Global::ID_EX.RegWrite = false;
	Global::ID_EX.MemRead = false;

	if (Global::Stall) {
		Instruction NOP;
		Global::ID_EX.inst = NOP;
		Global::ID_EX.Clear();
		return;
	}

	// Forwarding
	if (inst.fwdrs)
		DataRs = Global::MEM_WB.ALU_result;
	else
		DataRs = Global::reg[inst.rs];
	if (inst.fwdrt)
		DataRt = Global::MEM_WB.ALU_result;
	else
		DataRt = Global::reg[inst.rt];

	//Do ID things
	switch (inst.opcode) {
	case 0:
		switch (inst.funct) {
		case 32: // add
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 33: // addu
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 34: // sub
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 36: // and
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 37: // or
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 38: // xor
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 39: // nor
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 40: // nand
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 42: // slt
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 0: // sll
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 2: // srl
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 3: // sra
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
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
			Global::ID_EX.WriteDes = inst.rd;
			break;
		case 18: // mflo
			Global::ID_EX.RegWrite = true;
			Global::ID_EX.WriteDes = inst.rd;
			break;
		}
		break;
	case 8: // addi
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 9: // addiu
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 35: // lw
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 33: // lh
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 37: // lhu
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 32: // lb
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 36: // lbu
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.MemRead = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 15: // lui
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 12: // andi
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 13: // ori
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 14: // nori
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 10: // slti
		Global::ID_EX.RegWrite = true;
		Global::ID_EX.WriteDes = inst.rt;
		break;
	case 63: // Halt
		Global::Halt = true;
		break;
	case 4: // beq
		Global::Branch_taken = (DataRs == DataRt);
		NumberOverflowDetect(Global::PC + 4, Global::PC, 4);
		NumberOverflowDetect(Global::PC + 4 + 4 * (int)inst.imme, Global::PC + 4, 4 * (int)inst.imme);
		Global::Branch_PC = Global::PC + 4 * (int)inst.imme;
		break;
	case 5: // bne
		Global::Branch_taken = (DataRs != DataRt);
		NumberOverflowDetect(Global::PC + 4, Global::PC, 4);
		NumberOverflowDetect(Global::PC + 4 + 4 * (int)inst.imme, Global::PC + 4, 4 * (int)inst.imme);
		Global::Branch_PC = Global::PC + 4 * (int)inst.imme;
		break;
	case 7: // bgtz
		Global::Branch_taken = (DataRs > 0);
		NumberOverflowDetect(Global::PC + 4, Global::PC, 4);
		NumberOverflowDetect(Global::PC + 4 + 4 * (int)inst.imme, Global::PC + 4, 4 * (int)inst.imme);
		Global::Branch_PC = Global::PC + 4 * (int)inst.imme;
		break;
	case 2: // j
		inst.imme = (unsigned)(inst.line << 6) >> 6;
		jumpTaregt = ((Global::PC + 4) >> 28) << 28;
		jumpTaregt += (inst.imme << 2);
		Global::Branch_taken = true;
		Global::Branch_PC = jumpTaregt;
		break;
	case 3: // jal
		Global::ID_EX.WriteDes = 31;
		Global::ID_EX.ALU_result = Global::PC;
		inst.imme = (unsigned)(inst.line << 6) >> 6;
		jumpTaregt = ((Global::PC + 4) >> 28) << 28;
		jumpTaregt += (inst.imme << 2);
		Global::ID_EX.RegWrite = true;
		Global::Branch_taken = true;
		Global::Branch_PC = jumpTaregt;
		break;
	}
	if (!Global::Stall) Global::ID_EX.inst = inst;
	Global::ID_EX.RegRs = DataRs;
	Global::ID_EX.RegRt = DataRt;
}

void EXE() {
	Instruction inst = Global::ID_EX.inst;
	int ALU_result = 0, DataRs = 0, DataRt = 0;
	// Forwarding
	if (!isBranch(inst)) {
		if (inst.fwdrs)
		{
			DataRs = Global::MEM_WB.Data;
			if (inst.fwdrs_EX_DM_from)
				DataRs = Global::WB_AFTER.ALU_result;
		}
		else
			DataRs = Global::ID_EX.RegRs;
		if (inst.fwdrt)
		{
			DataRt = Global::MEM_WB.Data;
			if (inst.fwdrt_EX_DM_from)
				DataRt = Global::WB_AFTER.ALU_result;
		}
		else
			DataRt = Global::ID_EX.RegRt;
	}

	inst.fwdrs = false;
	inst.fwdrt = false;
	inst.fwdrt_EX_DM_from = false;
	inst.fwdrs_EX_DM_from = false;
	unsigned int A;
	unsigned long long x = 0;
	unsigned long long rs_64, rt_64;
	switch (inst.opcode) {
	case 0:
		switch (inst.funct) {
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
			ALU_result = DataRt << inst.shamt;
			break;
		case 2: // srl
			ALU_result = (unsigned int)DataRt >> inst.shamt;
			break;
		case 3: // sra
			ALU_result = DataRt >> inst.shamt;
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
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 9: // addiu
		ALU_result = DataRs + (int)inst.imme;
		break;
	case 35: // lw
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 33: // lh
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 37: // lhu
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 32: // lb
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 36: // lbu
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 43: // sw
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 41: // sh
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 40: // sb
		ALU_result = DataRs + (int)inst.imme;
		NumberOverflowDetect(ALU_result, DataRs, (int)inst.imme);
		break;
	case 15: // lui
		ALU_result = (int)inst.imme << 16;
		break;
	case 12: // andi
		A = (((unsigned int)inst.imme << 16) >> 16);
		ALU_result = DataRs & A;
		break;
	case 13: // ori
		A = (((unsigned int)inst.imme << 16) >> 16);
		ALU_result = DataRs | A;
		break;
	case 14: // nori
		A = (((unsigned int)inst.imme << 16) >> 16);
		ALU_result = ~(DataRs | A);
		break;
	case 10: // slti
		ALU_result = (DataRs < inst.imme);
		break;
	case 3: //jal
		ALU_result = Global::ID_EX.ALU_result;
		break;
	case 63: // Halt
		Global::Halt = true;
		break;
	}
	//Passing results
	Global::EX_MEM.WriteDes = Global::ID_EX.WriteDes;
	Global::EX_MEM.ALU_result = ALU_result;
	Global::EX_MEM.RegWrite = Global::ID_EX.RegWrite;
	Global::EX_MEM.inst = inst;
	Global::EX_MEM.MemRead = Global::ID_EX.MemRead;
	Global::EX_MEM.RegRt = DataRt;
}

void DM() {
	Instruction inst = Global::EX_MEM.inst;
	int Data = 0;

	switch (inst.opcode) {
	case 35: // lw
		DataMisalignedDetect(Global::EX_MEM.ALU_result, 4);
		MemOverflowDetect(Global::EX_MEM.ALU_result, 3);
		if (Global::error_toggle[1] || Global::error_toggle[2]) return;
		for (int i = 0; i < 4; i++)
			Data = (Data << 8) | (unsigned char)Global::Memory[Global::EX_MEM.ALU_result + i];
		break;
	case 33: // lh
		MemOverflowDetect(Global::EX_MEM.ALU_result, 1);
		DataMisalignedDetect(Global::EX_MEM.ALU_result, 2);
		if (Global::error_toggle[1] || Global::error_toggle[2]) return;
		Data = Global::Memory[Global::EX_MEM.ALU_result];
		Data = (Data << 8) | (unsigned char)Global::Memory[Global::EX_MEM.ALU_result + 1];
		break;
	case 37: // lhu
		MemOverflowDetect(Global::EX_MEM.ALU_result, 1);
		DataMisalignedDetect(Global::EX_MEM.ALU_result, 2);
		if (Global::error_toggle[1] || Global::error_toggle[2]) return;
		for (int i = 0; i < 2; i++)
			Data = (Data << 8) | (unsigned char)Global::Memory[Global::EX_MEM.ALU_result + i];
		break;
	case 32: // lb
		MemOverflowDetect(Global::EX_MEM.ALU_result, 0);
		if (Global::error_toggle[1]) return;
		Data = Global::Memory[Global::EX_MEM.ALU_result];
		break;
	case 36: // lbu
		MemOverflowDetect(Global::EX_MEM.ALU_result, 0);
		if (Global::error_toggle[1]) return;
		Data = (unsigned char)Global::Memory[Global::EX_MEM.ALU_result];
		break;
	case 43: // sw
		MemOverflowDetect(Global::EX_MEM.ALU_result, 3);
		DataMisalignedDetect(Global::EX_MEM.ALU_result, 4);
		if (Global::error_toggle[1] || Global::error_toggle[2]) return;
		Global::Memory[Global::EX_MEM.ALU_result] = (char)(Global::EX_MEM.RegRt >> 24);
		Global::Memory[Global::EX_MEM.ALU_result + 1] = (char)((Global::EX_MEM.RegRt >> 16) & 0xff);
		Global::Memory[Global::EX_MEM.ALU_result + 2] = (char)((Global::EX_MEM.RegRt >> 8) & 0xff);
		Global::Memory[Global::EX_MEM.ALU_result + 3] = (char)((Global::EX_MEM.RegRt) & 0xff);
		break;
	case 41: // sh
		MemOverflowDetect(Global::EX_MEM.ALU_result, 1);
		DataMisalignedDetect(Global::EX_MEM.ALU_result, 2);
		if (Global::error_toggle[1] || Global::error_toggle[2]) return;
		Global::Memory[Global::EX_MEM.ALU_result] = (char)((Global::EX_MEM.RegRt >> 8) & 0xff);
		Global::Memory[Global::EX_MEM.ALU_result + 1] = (char)((Global::EX_MEM.RegRt));
		break;
	case 40: // sb
		MemOverflowDetect(Global::EX_MEM.ALU_result, 0);
		if (Global::error_toggle[1]) return;
		Global::Memory[Global::EX_MEM.ALU_result] = (char)Global::EX_MEM.RegRt;
		break;
	default: // else
		Data = Global::EX_MEM.ALU_result;
		break;
	}
	Global::MEM_WB.Data = Data;
	Global::MEM_WB.MemRead = Global::EX_MEM.MemRead;
	Global::MEM_WB.inst = inst;
	Global::MEM_WB.RegWrite = Global::EX_MEM.RegWrite;
	Global::MEM_WB.RegRt = Global::EX_MEM.RegRt;
	Global::MEM_WB.ALU_result = Global::EX_MEM.ALU_result;
	Global::MEM_WB.WriteDes = Global::EX_MEM.WriteDes;


	//Global::debug();
}

void WB() {
	Instruction inst = Global::MEM_WB.inst;
	if (Global::MEM_WB.RegWrite) {
		if ((Global::MEM_WB.WriteDes == 0) && !isBranch(inst) && !notS(inst.name)) {
			Global::error_toggle[0] = true;		//Write 0
			return;
		}
		if (inst.type == 'R' || inst.type == 'J')
		{
			Global::reg[Global::MEM_WB.WriteDes] = Global::MEM_WB.ALU_result;
			Global::WB_AFTER.ALU_result = Global::MEM_WB.ALU_result;
		}
		else if (inst.type == 'I')
		{
			Global::reg[Global::MEM_WB.WriteDes] = Global::MEM_WB.Data;
			Global::WB_AFTER.ALU_result = Global::MEM_WB.Data;
		}
	}
}
