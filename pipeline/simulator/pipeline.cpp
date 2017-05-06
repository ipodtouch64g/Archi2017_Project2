#include <iostream>
#include "Simulator.h"
#include "pipeline.h"

using namespace std;
inline void decodeLine(Instruction &inst)
{
	inst.line = Simulator::Address[Simulator::PC];
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
		Simulator::error_toggle[3] = true;
}

inline void HILOWriteDetect(string op) {
	if (op == "mult" || op == "multu")
	{
		if (Simulator::toggle_MULT == 1 && Simulator::toggle_HILO == 1) //NO HILO error , turn off HILO toggle now
		{
			Simulator::toggle_HILO = 0;
		}
		else if (Simulator::toggle_MULT == 1 && Simulator::toggle_HILO == 0) //HILO error!!
		{
			Simulator::error_toggle[4] = 1;
		}
		//NEED TO DETECT HILO ON AND TURN IT OFF BEFORE FIRST MULT

		else {
			Simulator::toggle_MULT = 1; Simulator::toggle_HILO = 0;
		}
	}
	else //op == hi lo
	{
		Simulator::toggle_HILO = 1;
	}
}

inline void MemOverflowDetect(int a, int len_byte) {
	if (a < 0 || a >= 1024 || a + len_byte < 0 || a + len_byte >= 1024)
		Simulator::error_toggle[1] = true;
}

inline void DataMisalignedDetect(int a, int len_byte) {
	if (a % len_byte != 0)
		Simulator::error_toggle[2] = true;
}

inline void checkfwdinID(Instruction &inst)
{
	int RsData, RtData;
	if (Simulator::isBranch(inst)) {
		bool Branching = false;
		// Forward rs
		if (!Simulator::EX_MEM.MemRead && Simulator::EX_MEM.RegWrite && (Simulator::EX_MEM.WriteDes != 0) && (Simulator::EX_MEM.WriteDes == inst.rs)) {
			RsData = Simulator::EX_MEM.ALU_result;
			Simulator::IF_ID.inst.fwdrs = true;
		}
		else {
			Simulator::IF_ID.inst.fwdrs = false;
			if (Simulator::MEM_WB.RegWrite && (Simulator::MEM_WB.inst.name != "NOP") && (Simulator::MEM_WB.WriteDes == inst.rs) && Simulator::MEM_WB.WriteDes != 0) {
				if (Simulator::MEM_WB.inst.type == 'R')
					RsData = Simulator::MEM_WB.ALU_result;
				else if (Simulator::MEM_WB.inst.type == 'I')
					RsData = Simulator::MEM_WB.Data;
			}
			else
				RsData = Simulator::reg[inst.rs];
		}
		// Forward rt
		if (!Simulator::EX_MEM.MemRead && Simulator::EX_MEM.RegWrite && (Simulator::EX_MEM.WriteDes != 0) && (Simulator::EX_MEM.WriteDes == inst.rt)) {
			if (!Simulator::isJ(inst.name)) {
				Simulator::IF_ID.inst.fwdrt = true;
				RtData = Simulator::EX_MEM.ALU_result;
			}
		}
		else {
			Simulator::IF_ID.inst.fwdrt = false;
			if (Simulator::MEM_WB.RegWrite && Simulator::MEM_WB.WriteDes == inst.rt && Simulator::MEM_WB.WriteDes != 0) {
				if (Simulator::MEM_WB.inst.type == 'R')
					RtData = Simulator::MEM_WB.ALU_result;
				else if (Simulator::MEM_WB.inst.type == 'I')
					RtData = Simulator::MEM_WB.Data;
			}
			else
				RtData = Simulator::reg[inst.rt];
		}
		switch (inst.opcode) {
		case 4: // beq
			Branching = (RsData == RtData);
			break;
		case 5: // bne
			Branching = (RsData != RtData);
			break;
		case 7: // bgtz
			Branching = (RsData > 0);
			break;
		case 2: // j
			Branching = true;
			break;
		case 3: // jal
			Branching = true;
			break;
		case 0: // jg
			Branching = true;
		}
		Simulator::Flush = (Branching) ? true : false;
	}
	else {
		Simulator::IF_ID.inst.fwdrs = false;
		Simulator::IF_ID.inst.fwdrt = false;
	}
}

inline void checkfwdinEX(Instruction &inst)
{
	if (!Simulator::isBranch(inst)&& !Simulator::isHILO(inst)&&inst.name!="HALT") {
		/*
		Check for fwdrs
		*/
		if (!Simulator::EX_MEM.MemRead && Simulator::EX_MEM.RegWrite && (Simulator::EX_MEM.WriteDes != 0) && (Simulator::EX_MEM.WriteDes == inst.rs)) {
			if (inst.name != "SLL" && inst.name != "SRL" && inst.name != "SRA" && inst.name != "LUI")
			{
				Simulator::ID_EX.inst.fwdrs = true;
				Simulator::ID_EX.inst.fwdrs_EX_DM_from = false;
			}

		}

		else {
			Simulator::ID_EX.inst.fwdrs = false;
			Simulator::ID_EX.inst.fwdrs_EX_DM_from = false;
		}

		/*
		Check for fwdrt
		*/
		if (!Simulator::EX_MEM.MemRead && Simulator::EX_MEM.RegWrite && (Simulator::EX_MEM.WriteDes != 0))
		{
			if (inst.type == 'I')
			{
				if (inst.name == "SW" || inst.name == "SB" || inst.name == "SH")
				{
					if (Simulator::EX_MEM.WriteDes == inst.rt)
					{
						Simulator::ID_EX.inst.fwdrt = true;
						Simulator::ID_EX.inst.fwdrt_EX_DM_from = false;
					}
				}
			}
			else
			{
				if (Simulator::EX_MEM.WriteDes == inst.rt)
				{
					Simulator::ID_EX.inst.fwdrt = true;
					Simulator::ID_EX.inst.fwdrt_EX_DM_from = false;
				}
			}
		}
		if (Simulator::MEM_WB.RegWrite && (Simulator::MEM_WB.WriteDes != 0) && ((Simulator::MEM_WB.WriteDes == inst.rt) || (Simulator::MEM_WB.WriteDes == inst.rs)))
		{
			if (Simulator::MEM_WB.WriteDes == inst.rs)
			{
				if (inst.name != "SLL" && inst.name != "SRL" && inst.name != "SRA" && inst.name != "LUI")
				{
					if (!Simulator::ID_EX.inst.fwdrs)		//prevent double hazard
					{
						Simulator::ID_EX.inst.fwdrs = true;
						Simulator::ID_EX.inst.fwdrs_EX_DM_from = true;
					}
				}
			}
			if (Simulator::MEM_WB.WriteDes == inst.rt)
			{
				if (inst.type == 'I')
				{
					if (inst.name == "SW" || inst.name == "SB" || inst.name == "SH")
					{
						if (!Simulator::ID_EX.inst.fwdrt)		//prevent double hazard
						{
							Simulator::ID_EX.inst.fwdrt = true;
							Simulator::ID_EX.inst.fwdrt_EX_DM_from = true;
						}
					}
				}
				else
				{
					if (!Simulator::ID_EX.inst.fwdrt)		//prevent double hazard
					{
						Simulator::ID_EX.inst.fwdrt = true;
						Simulator::ID_EX.inst.fwdrt_EX_DM_from = true;
					}
				}
			}
		}
	}
	else {
		Simulator::ID_EX.inst.fwdrs = false;
		Simulator::ID_EX.inst.fwdrt = false;
		Simulator::ID_EX.inst.fwdrs_EX_DM_from = false;
		Simulator::ID_EX.inst.fwdrt_EX_DM_from = false;
	}
}
inline void checkstall(Instruction &inst)
{
	if(!Simulator::isHILO(inst))
	{
		if (!Simulator::isBranch(inst))
		{
			if (Simulator::ID_EX.MemRead && Simulator::ID_EX.WriteDes != 0 && ((Simulator::ID_EX.WriteDes == inst.rs) || (Simulator::ID_EX.WriteDes == inst.rt)))
			{
				if (Simulator::ID_EX.WriteDes == inst.rs && inst.name!="LUI" && !Simulator::isShift(inst))
				{
					{
						Simulator::Stall = true;
					}
				}
				else if (Simulator::ID_EX.WriteDes == inst.rt) 
				{
					if (inst.type == 'I')
					{
						if (inst.name == "SW" || inst.name == "SH" || inst.name == "SB")
						{
							Simulator::Stall = true;
						}

					}
					else if (inst.type != 'I')
					{
						Simulator::Stall = true;
					}
				}
			}
		}
		else 
		{
			if (Simulator::ID_EX.RegWrite && (Simulator::ID_EX.WriteDes != 0) && ((Simulator::ID_EX.WriteDes == inst.rs) || (Simulator::ID_EX.WriteDes == inst.rt))) 
			{
				if (Simulator::ID_EX.WriteDes == inst.rs)
				{
					if (inst.name != "J" && inst.name != "JAL")
					{
						Simulator::Stall = true;
					}
				}
				if (Simulator::ID_EX.WriteDes == inst.rt)
				{
					if (!Simulator::isJ(inst.name))
					{
						Simulator::Stall = true;
					}
				}
			}
			else if (Simulator::EX_MEM.MemRead && (Simulator::EX_MEM.WriteDes != 0) && ((Simulator::EX_MEM.WriteDes == inst.rs) || (Simulator::EX_MEM.WriteDes == inst.rt)))
			{
				Simulator::Stall = true;
			}
		}
	}
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
	if (!Simulator::Stall) Simulator::IF_ID.inst = inst;
}

void ID() {
	Instruction inst = Simulator::IF_ID.inst;
	int jumpTarget = 0, DataRs, DataRt;
	Simulator::ID_EX.RegWrite = false;
	Simulator::ID_EX.MemRead = false;

	if (Simulator::Stall) {
		Instruction NOP;
		Simulator::ID_EX.inst = NOP;
		Simulator::ID_EX.ALU_result = 0;
		Simulator::ID_EX.Data = 0;
		Simulator::ID_EX.RegRs = 0;
		Simulator::ID_EX.RegRt = 0;
		Simulator::ID_EX.WriteDes = 0;
		Simulator::ID_EX.RegWrite = false;
		Simulator::ID_EX.MemRead = false;
		return;
	}

	// Forwarding
	if (inst.fwdrs)
		DataRs = Simulator::MEM_WB.ALU_result;
	else
		DataRs = Simulator::reg[inst.rs];
	if (inst.fwdrt)
		DataRt = Simulator::MEM_WB.ALU_result;
	else
		DataRt = Simulator::reg[inst.rt];

	//Do ID things
	switch (inst.opcode) {
	case 0:
		switch (inst.funct) {
		case 32: // add
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 33: // addu
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 34: // sub
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 36: // and
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 37: // or
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 38: // xor
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 39: // nor
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 40: // nand
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 42: // slt
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 0: // sll
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 2: // srl
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 3: // sra
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 8: // jr
			Simulator::Branch_taken = true;
			Simulator::Branch_PC = DataRs;
			break;
		case 24: // mult
			Simulator::ID_EX.RegWrite = false;
			break;
		case 25: // multu
			Simulator::ID_EX.RegWrite = false;
			break;
		case 16: // mfhi
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		case 18: // mflo
			Simulator::ID_EX.RegWrite = true;
			Simulator::ID_EX.WriteDes = inst.rd;
			break;
		}
		break;
	case 8: // addi
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 9: // addiu
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 35: // lw
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.MemRead = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 33: // lh
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.MemRead = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 37: // lhu
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.MemRead = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 32: // lb
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.MemRead = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 36: // lbu
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.MemRead = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 15: // lui
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 12: // andi
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 13: // ori
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 14: // nori
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 10: // slti
		Simulator::ID_EX.RegWrite = true;
		Simulator::ID_EX.WriteDes = inst.rt;
		break;
	case 63: // Halt
		Simulator::Halt = true;
		break;
	case 4: // beq
		Simulator::Branch_taken = (DataRs == DataRt);
		NumberOverflowDetect(Simulator::PC + 4, Simulator::PC, 4);
		NumberOverflowDetect(Simulator::PC + 4 + 4 * (int)inst.imme, Simulator::PC + 4, 4 * (int)inst.imme);
		Simulator::Branch_PC = Simulator::PC + 4 * (int)inst.imme;
		break;
	case 5: // bne
		Simulator::Branch_taken = (DataRs != DataRt);
		NumberOverflowDetect(Simulator::PC + 4, Simulator::PC, 4);
		NumberOverflowDetect(Simulator::PC + 4 + 4 * (int)inst.imme, Simulator::PC + 4, 4 * (int)inst.imme);
		Simulator::Branch_PC = Simulator::PC + 4 * (int)inst.imme;
		break;
	case 7: // bgtz
		Simulator::Branch_taken = (DataRs > 0);
		NumberOverflowDetect(Simulator::PC + 4, Simulator::PC, 4);
		NumberOverflowDetect(Simulator::PC + 4 + 4 * (int)inst.imme, Simulator::PC + 4, 4 * (int)inst.imme);
		Simulator::Branch_PC = Simulator::PC + 4 * (int)inst.imme;
		break;
	case 2: // j
		inst.imme = (unsigned)(inst.line << 6) >> 6;
		jumpTarget = ((Simulator::PC + 4) >> 28) << 28;
		jumpTarget += (inst.imme << 2);
		Simulator::Branch_taken = true;
		Simulator::Branch_PC = jumpTarget;
		break;
	case 3: // jal
		Simulator::ID_EX.WriteDes = 31;
		Simulator::ID_EX.ALU_result = Simulator::PC;
		inst.imme = (unsigned)(inst.line << 6) >> 6;
		jumpTarget = ((Simulator::PC + 4) >> 28) << 28;
		jumpTarget += (inst.imme << 2);
		Simulator::ID_EX.RegWrite = true;
		Simulator::Branch_taken = true;
		Simulator::Branch_PC = jumpTarget;
		break;
	}
	if (!Simulator::Stall) Simulator::ID_EX.inst = inst;
	//if (Simulator::Branch_taken) Simulator::Flush = true;
	Simulator::ID_EX.RegRs = DataRs;
	Simulator::ID_EX.RegRt = DataRt;
}

void EXE() {
	Instruction inst = Simulator::ID_EX.inst;
	int ALU_result = 0, DataRs = 0, DataRt = 0;
	// Forwarding
	if (!Simulator::isBranch(inst)) {
		if (inst.fwdrs)
		{
			DataRs = Simulator::MEM_WB.Data;
			if (inst.fwdrs_EX_DM_from)
				DataRs = Simulator::WB_AFTER.ALU_result;
		}
		else
			DataRs = Simulator::ID_EX.RegRs;
		if (inst.fwdrt)
		{
			DataRt = Simulator::MEM_WB.Data;
			if (inst.fwdrt_EX_DM_from)
				DataRt = Simulator::WB_AFTER.ALU_result;
		}
		else
			DataRt = Simulator::ID_EX.RegRt;
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
			ALU_result = Simulator::ID_EX.ALU_result;
			HILOWriteDetect("mult");
			x = (long long)DataRs*(long long)DataRt;
			Simulator::HI = x >> 32;
			Simulator::LO = x << 32 >> 32;
			break;
		case 25:// multu
			ALU_result = Simulator::ID_EX.ALU_result;
			HILOWriteDetect("multu");
			rs_64 = (unsigned)DataRs;
			rt_64 = (unsigned)DataRt;
			x = rs_64*rt_64;
			Simulator::HI = x >> 32;
			Simulator::LO = x << 32 >> 32;
			break;
		case 16:// mfhi
			ALU_result = Simulator::HI;
			HILOWriteDetect("mfhi");
			break;
		case 18:// mflo
			ALU_result = Simulator::LO;
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
		ALU_result = Simulator::ID_EX.ALU_result;
		break;
	case 63: // Halt
		Simulator::Halt = true;
		break;
	}
	//Passing results
	Simulator::EX_MEM.WriteDes = Simulator::ID_EX.WriteDes;
	Simulator::EX_MEM.ALU_result = ALU_result;
	Simulator::EX_MEM.RegWrite = Simulator::ID_EX.RegWrite;
	Simulator::EX_MEM.inst = inst;
	Simulator::EX_MEM.MemRead = Simulator::ID_EX.MemRead;
	Simulator::EX_MEM.RegRt = DataRt;
}

void DM() {
	Instruction inst = Simulator::EX_MEM.inst;
	int Data = 0;

	switch (inst.opcode) {
	case 35: // lw
		DataMisalignedDetect(Simulator::EX_MEM.ALU_result, 4);
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 3);
		if (Simulator::error_toggle[1] || Simulator::error_toggle[2]) return;
		for (int i = 0; i < 4; i++)
			Data = (Data << 8) | (unsigned char)Simulator::Memory[Simulator::EX_MEM.ALU_result + i];
		break;
	case 33: // lh
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 1);
		DataMisalignedDetect(Simulator::EX_MEM.ALU_result, 2);
		if (Simulator::error_toggle[1] || Simulator::error_toggle[2]) return;
		Data = Simulator::Memory[Simulator::EX_MEM.ALU_result];
		Data = (Data << 8) | (unsigned char)Simulator::Memory[Simulator::EX_MEM.ALU_result + 1];
		break;
	case 37: // lhu
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 1);
		DataMisalignedDetect(Simulator::EX_MEM.ALU_result, 2);
		if (Simulator::error_toggle[1] || Simulator::error_toggle[2]) return;
		for (int i = 0; i < 2; i++)
			Data = (Data << 8) | (unsigned char)Simulator::Memory[Simulator::EX_MEM.ALU_result + i];
		break;
	case 32: // lb
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 0);
		if (Simulator::error_toggle[1]) return;
		Data = Simulator::Memory[Simulator::EX_MEM.ALU_result];
		break;
	case 36: // lbu
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 0);
		if (Simulator::error_toggle[1]) return;
		Data = (unsigned char)Simulator::Memory[Simulator::EX_MEM.ALU_result];
		break;
	case 43: // sw
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 3);
		DataMisalignedDetect(Simulator::EX_MEM.ALU_result, 4);
		if (Simulator::error_toggle[1] || Simulator::error_toggle[2]) return;
		Simulator::Memory[Simulator::EX_MEM.ALU_result] = (char)(Simulator::EX_MEM.RegRt >> 24);
		Simulator::Memory[Simulator::EX_MEM.ALU_result + 1] = (char)((Simulator::EX_MEM.RegRt >> 16) & 0xff);
		Simulator::Memory[Simulator::EX_MEM.ALU_result + 2] = (char)((Simulator::EX_MEM.RegRt >> 8) & 0xff);
		Simulator::Memory[Simulator::EX_MEM.ALU_result + 3] = (char)((Simulator::EX_MEM.RegRt) & 0xff);
		break;
	case 41: // sh
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 1);
		DataMisalignedDetect(Simulator::EX_MEM.ALU_result, 2);
		if (Simulator::error_toggle[1] || Simulator::error_toggle[2]) return;
		Simulator::Memory[Simulator::EX_MEM.ALU_result] = (char)((Simulator::EX_MEM.RegRt >> 8) & 0xff);
		Simulator::Memory[Simulator::EX_MEM.ALU_result + 1] = (char)((Simulator::EX_MEM.RegRt));
		break;
	case 40: // sb
		MemOverflowDetect(Simulator::EX_MEM.ALU_result, 0);
		if (Simulator::error_toggle[1]) return;
		Simulator::Memory[Simulator::EX_MEM.ALU_result] = (char)Simulator::EX_MEM.RegRt;
		break;
	default: // else
		Data = Simulator::EX_MEM.ALU_result;
		break;
	}
	Simulator::MEM_WB.Data = Data;
	Simulator::MEM_WB.MemRead = Simulator::EX_MEM.MemRead;
	Simulator::MEM_WB.inst = inst;
	Simulator::MEM_WB.RegWrite = Simulator::EX_MEM.RegWrite;
	Simulator::MEM_WB.RegRt = Simulator::EX_MEM.RegRt;
	Simulator::MEM_WB.ALU_result = Simulator::EX_MEM.ALU_result;
	Simulator::MEM_WB.WriteDes = Simulator::EX_MEM.WriteDes;


	//Simulator::debug();
}

void WB() {
	Instruction inst = Simulator::MEM_WB.inst;
	if (Simulator::MEM_WB.RegWrite) {
		if ((Simulator::MEM_WB.WriteDes == 0) && !Simulator::isBranch(inst) && !Simulator::notS(inst.name)) {
			Simulator::error_toggle[0] = true;		//Write 0
			return;
		}
		if (inst.type == 'R' || inst.type == 'J')
		{
			Simulator::reg[Simulator::MEM_WB.WriteDes] = Simulator::MEM_WB.ALU_result;
			Simulator::WB_AFTER.ALU_result = Simulator::MEM_WB.ALU_result;
		}
		else if (inst.type == 'I')
		{
			Simulator::reg[Simulator::MEM_WB.WriteDes] = Simulator::MEM_WB.Data;
			Simulator::WB_AFTER.ALU_result = Simulator::MEM_WB.Data;
		}
	}
}
void nextStage() {
	
	Instruction inst;
	Simulator::Stall = false;
	Simulator::IF_ID.inst.fwdrs = false;
	Simulator::IF_ID.inst.fwdrt = false;
	Simulator::ID_EX.inst.fwdrs = false;
	Simulator::ID_EX.inst.fwdrt = false;
	Simulator::ID_EX.inst.fwdrs_EX_DM_from = false;
	Simulator::ID_EX.inst.fwdrt_EX_DM_from = false;
	
	//Forward in ID (only when branch) 
	inst = Simulator::IF_ID.inst;
	checkfwdinID(inst);
	//Forwarding in EX (R_type forwarding from EX/MEM or MEM/WB)
	inst = Simulator::ID_EX.inst;
	checkfwdinEX(inst);
	
	// Stall Test
	inst = Simulator::IF_ID.inst;
	if (inst.name == "NOP" || inst.name == "HALT") return;
	checkstall(inst);
	
}