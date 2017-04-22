#include <iostream>
#include <fstream>
#include <map>
#include <iomanip>
#include <string>
#include <vector>

#include "Simulator.h"
#include "Stage.h"

using namespace std;

string errorMsg[5] = { ": Write $0 Error", ": Address Overflow", ": Misalignment Error", ": Number Overflow",": Overwrite HI-LO registers" };
string stageNames[5] = { "IF: ", "ID: ", "EX: ", "DM: ", "WB: " };

inline bool isJ(string s) {
	return (s == "JR" || s == "BGTZ" || s == "J" || s == "JAL");
}

inline bool isLoad(string s) {
	return (s == "LW" || s == "LH" || s == "LHU" || s == "LB" || s == "LBU");
}

inline bool isBranchTo(Instruction ins) {
	return ((ins.Name == "BEQ") || (ins.Name == "BNE") || (ins.Name == "BGTZ") || (ins.Name == "J") || (ins.Name == "JAL") || (ins.Name == "JR"));
}

void nextStage() {
	Instruction ins = Global::IF_ID.ins;

	Global::Stall = false;
	Global::IF_ID.ins.fwdrs = false;
	Global::IF_ID.ins.fwdrt = false;
	Global::ID_EX.ins.fwdrs = false;
	Global::ID_EX.ins.fwdrt = false;
	Global::ID_EX.ins.fwdrs_EX_DM_from = false;
	Global::ID_EX.ins.fwdrt_EX_DM_from = false;
	int RsData, RtData;

	/*
	*	Forward in ID (only when branch)
	*/
	if (isBranchTo(ins)) {
		bool Branching = false;
		// Forward rs
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0) && (Global::EX_MEM.WriteDes == ins.rs)) {
			Global::IF_ID.ins.fwdrs = true;
			RsData = Global::EX_MEM.ALU_result;
		}
		else {
			Global::IF_ID.ins.fwdrs = false;
			if (Global::MEM_WB.RegWrite && (Global::MEM_WB.ins.Name != "NOP") && (Global::MEM_WB.WriteDes == ins.rs) && Global::MEM_WB.WriteDes != 0) {
				if (Global::MEM_WB.ins.type == 'R')
					RsData = Global::MEM_WB.ALU_result;
				else if (Global::MEM_WB.ins.type == 'I')
					RsData = Global::MEM_WB.Data;
			}
			else
				RsData = Global::reg[ins.rs];
		}
		// Forward rt
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0) && (Global::EX_MEM.WriteDes == ins.rt)) {
			if (!isJ(ins.Name)) {
				Global::IF_ID.ins.fwdrt = true;
				RtData = Global::EX_MEM.ALU_result;
			}
		}
		else {
			Global::IF_ID.ins.fwdrt = false;
			if (Global::MEM_WB.RegWrite && Global::MEM_WB.WriteDes == ins.rt && Global::MEM_WB.WriteDes != 0) {
				if (Global::MEM_WB.ins.type == 'R')
					RtData = Global::MEM_WB.ALU_result;
				else if (Global::MEM_WB.ins.type == 'I')
					RtData = Global::MEM_WB.Data;
			}
			else
				RtData = Global::reg[ins.rt];
		}
		switch (ins.opcode) {
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
		Global::Flush = (Branching) ? true : false;
	}
	else {
		Global::IF_ID.ins.fwdrs = false;
		Global::IF_ID.ins.fwdrt = false;
	}

	/*
	   Forwarding in EX (R_type forwarding from EX/MEM or MEM/WB)
	 */
	ins = Global::ID_EX.ins;
	if (!isBranchTo(ins)) {
		/*
		   Check for fwdrs
		 */
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0) && (Global::EX_MEM.WriteDes == ins.rs)) {
			if (ins.Name != "SLL" && ins.Name != "SRL" && ins.Name != "SRA" && ins.Name != "LUI")
			{
				Global::ID_EX.ins.fwdrs = true;
				Global::ID_EX.ins.fwdrs_EX_DM_from = false;
			}

		}

		else {
			Global::ID_EX.ins.fwdrs = false;
			Global::ID_EX.ins.fwdrs_EX_DM_from = false;
		}

		/*
		   Check for fwdrt
		 */
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0))
		{
			if (ins.type == 'I')
			{
				if (ins.Name == "SW" || ins.Name == "SB" || ins.Name == "SH")
				{
					if (Global::EX_MEM.WriteDes == ins.rt)
					{
						Global::ID_EX.ins.fwdrt = true;
						Global::ID_EX.ins.fwdrt_EX_DM_from = false;
					}
				}
			}
			else
			{
				if (Global::EX_MEM.WriteDes == ins.rt)
				{
					Global::ID_EX.ins.fwdrt = true;
					Global::ID_EX.ins.fwdrt_EX_DM_from = false;
				}
			}
		}
		if (Global::MEM_WB.RegWrite && (Global::MEM_WB.WriteDes != 0) && ((Global::MEM_WB.WriteDes == ins.rt) || (Global::MEM_WB.WriteDes == ins.rs)))
		{
			if (Global::MEM_WB.WriteDes == ins.rs)
			{
				if (ins.Name != "SLL" && ins.Name != "SRL" && ins.Name != "SRA" && ins.Name != "LUI")
				{
					if (!Global::ID_EX.ins.fwdrs)		//prevent double hazard
					{
						Global::ID_EX.ins.fwdrs = true;
						Global::ID_EX.ins.fwdrs_EX_DM_from = true;
					}
				}
			}
			else if (Global::MEM_WB.WriteDes == ins.rt)
			{
				if (ins.type == 'I')
				{
					if (ins.Name == "SW" || ins.Name == "SB" || ins.Name == "SH")
					{
						if (!Global::ID_EX.ins.fwdrt)		//prevent double hazard
						{
							Global::ID_EX.ins.fwdrt = true;
							Global::ID_EX.ins.fwdrt_EX_DM_from = true;
						}
					}
				}
				else
				{
					if (!Global::ID_EX.ins.fwdrt)		//prevent double hazard
					{
						Global::ID_EX.ins.fwdrt = true;
						Global::ID_EX.ins.fwdrt_EX_DM_from = true;
					}
				}
			}
		}
	}
	else {
		Global::ID_EX.ins.fwdrs = false;
		Global::ID_EX.ins.fwdrt = false;
		Global::ID_EX.ins.fwdrs_EX_DM_from = false;
		Global::ID_EX.ins.fwdrt_EX_DM_from = false;
	}

	// Stall Test
	ins = Global::IF_ID.ins;
	if (ins.Name == "NOP" || ins.Name == "HALT") return;
	if (!isBranchTo(ins)) {
		if (Global::ID_EX.MemRead && Global::ID_EX.WriteDes != 0 && ((Global::ID_EX.WriteDes == ins.rs) || (Global::ID_EX.WriteDes == ins.rt))) {
			if (Global::ID_EX.WriteDes == ins.rs) {
				{
					Global::Stall = true;
				}
			}
			else if (Global::ID_EX.WriteDes == ins.rt) {
				if (ins.type == 'I') {
					if (ins.Name == "SW" || ins.Name == "SH" || ins.Name == "SB")
					{
						Global::Stall = true;
					}
				}
				else if (ins.type != 'I')
				{
					Global::Stall = true;
				}
			}
		}
	}
	else {
		if (Global::ID_EX.RegWrite && (Global::ID_EX.WriteDes != 0) && ((Global::ID_EX.WriteDes == ins.rs) || (Global::ID_EX.WriteDes == ins.rt))) {
			if (Global::ID_EX.WriteDes == ins.rs) {
				if (ins.Name != "J" && ins.Name != "JAL")
				{
					Global::Stall = true;
				}
			}
			if (Global::ID_EX.WriteDes == ins.rt) {
				if (!isJ(ins.Name))
				{
					Global::Stall = true;
				}
			}
		}
		else if (Global::EX_MEM.MemRead && (Global::EX_MEM.WriteDes != 0) && ((Global::EX_MEM.WriteDes == ins.rs) || (Global::EX_MEM.WriteDes == ins.rt)))
		{
			Global::Stall = true;
		}
	}
}

void Initialize() {
	for (int i = 0; i < 32; i++)
		Global::reg[i] = 0;
	for (int i = 0; i < 32; i++)
		lastThings::reg[i] = 0;
	for (int i = 0; i < 1024; i++)
		Global::Address[i] = 0;
	Global::LO = 0; Global::HI = 0;
	lastThings::LO = 0; lastThings::HI = 0;
}

void printCycle(ofstream &snapshot, int &Cycle) {
	snapshot << "cycle " << dec << Cycle++ << endl;

	for (int i = 0; i < 32; i++) {
		if (Global::reg[i] != lastThings::reg[i] || Cycle == 1)
		{
			snapshot << "$" << setw(2) << setfill('0') << dec << i;
			snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::reg[i] << endl;
		}
		lastThings::reg[i] = Global::reg[i];
	}
	if (Global::HI != lastThings::HI || Cycle == 1)
	{
		snapshot << "$" << setw(2) << "HI";
		snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::HI << endl;
	}
	lastThings::HI = Global::HI;
	if (Global::LO != lastThings::LO || Cycle == 1)
	{
		snapshot << "$" << setw(2) << "LO";
		snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::LO << endl;
	}
	lastThings::LO = Global::LO;
	snapshot << "PC: 0x" << setw(8) << setfill('0') << hex << uppercase << Global::PC << endl;
	snapshot << stageNames[0] << "0x" << setw(8) << setfill('0') << hex << uppercase << Global::Address[Global::PC];
	if (Global::Stall)
		snapshot << " to_be_stalled";
	else if (Global::Flush)
		snapshot << " to_be_flushed";
	snapshot << endl;
	if ((unsigned int)Global::Address[Global::PC] != 0xFFFFFFFF)
		Global::Halt = false;

	Buffer Buf;
	for (int i = 0; i < 4; i++) {
		if (i == 0) Buf = Global::IF_ID;
		else if (i == 1) Buf = Global::ID_EX;
		else if (i == 2) Buf = Global::EX_MEM;
		else Buf = Global::MEM_WB;

		if (Buf.ins.Name != "HALT")
			Global::Halt = false;

		int op = ((unsigned int)Buf.ins.line) >> 26;
		int funct = ((unsigned int)(Buf.ins.line << 26)) >> 26;
		bool R_type = (op == 0 && funct == 0);
		snapshot << stageNames[i + 1];

		if (R_type && (Buf.ins.rt == 0) && (Buf.ins.rd == 0) && (Buf.ins.shamt == 0))
			snapshot << "NOP" << endl;

		else if (Global::Stall && i == 0)
			snapshot << Buf.ins.Name << " to_be_stalled" << endl;

		else if (Buf.ins.fwdrs) {
			if (Buf.ins.fwdrs_EX_DM_from)
				snapshot << Buf.ins.Name << " fwd_DM-WB_rs_$" << dec << Buf.ins.rs;
			else
				snapshot << Buf.ins.Name << " fwd_EX-DM_rs_$" << dec << Buf.ins.rs;
			if (Buf.ins.fwdrt)
			{
				if (Buf.ins.fwdrt_EX_DM_from)
					snapshot << " fwd_DM-WB_rt_$" << dec << Buf.ins.rt;
				else
					snapshot << " fwd_EX-DM_rt_$" << dec << Buf.ins.rt;
			}
			snapshot << endl;
		}
		else if (Buf.ins.fwdrt) {
			if (Buf.ins.fwdrt_EX_DM_from)
				snapshot << Buf.ins.Name << " fwd_DM-WB_rt_$" << dec << Buf.ins.rt;
			else
				snapshot << Buf.ins.Name << " fwd_EX-DM_rt_$" << dec << Buf.ins.rt;
			snapshot << endl;
		}
		else
			snapshot << Buf.ins.Name << endl;
	}
	snapshot << endl << endl;
}

int main() {
	char ch;
	int line = 0, Cycle = 0;

	Initialize();

	ofstream snapshot("snapshot.rpt", ios::out);
	ofstream error_dump("error_dump.rpt", ios::out);
	ifstream image("iimage.bin", ios::in | ios::binary);

	// PC
	for (int i = 0; i < 4; i++) {
		image.get(ch);
		line = (line << 8) | (unsigned char)ch;
	}
	Global::PC = line;
	// #Lines
	for (int i = 0; i < 4; i++) {
		image.get(ch);
		line = (line << 8) | (unsigned char)ch;
	}
	int Num = line;
	// Ins
	for (int i = 0; i < Num; i++) {
		for (int j = 0; j < 4; j++) {
			image.get(ch);
			line = (line << 8) | (unsigned char)ch;
		}
		Global::Address[Global::PC + i * 4] = line;
	}

	image.close();

	// dimage
	image.open("dimage.bin", ios::in | ios::binary);
	// $sp
	for (int i = 4; i > 0; i--) {
		image.get(ch);
		line = (line << 8) + (unsigned char)ch;
	}
	Global::reg[29] = line;
	// #Lines
	for (int i = 4; i > 0; i--) {
		image.get(ch);
		line = (line << 8) + (unsigned char)ch;
	}
	int index = line;
	for (int i = 0; i < index * 4; i++) {
		image.get(ch);
		Global::Memory[i] = ch;
	}
	//Print 1st time
	printCycle(snapshot, Cycle);
	Global::Halt = false;
	Global::Stall = false;

	//Start Instructions
	while (!Global::Halt) {
		Global::Branch_taken = false;
		Global::Flush = false;
		for (int i = 0; i < 5; i++) Global::error_type[i] = false;
		WB();
		DM();
		EXE();
		ID();
		IF();
		// Print errors
		for (int i = 0; i < 5; i++) {
			if (Global::error_type[i])
				error_dump << "In cycle " << Cycle << errorMsg[i] << endl;
		}

		// Halt if misaligned or addovf
		if (Global::error_type[1] || Global::error_type[2]) break;

		// Branch taken then flush
		if (!Global::Stall && Global::Branch_taken) {
			Instruction NOP;
			Global::PC = Global::Branch_PC;
			Global::IF_ID.ins = NOP;
		}

		else if (!Global::Stall)
			Global::PC += 4;

		nextStage();
		printCycle(snapshot, Cycle);

		if (Global::Halt == true)
			break;

	}

	snapshot.close();
	error_dump.close();
	return 0;
}
