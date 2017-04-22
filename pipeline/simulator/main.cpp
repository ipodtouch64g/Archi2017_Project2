#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>


#include "Simulator.h"
#include "pipeline.h"

using namespace std;

string errorMsg[5] = { ": Write $0 Error", ": Address Overflow", ": Misalignment Error", ": Number Overflow",": Overwrite HI-LO registers" };
string stagenames[5] = { "IF: ", "ID: ", "EX: ", "DM: ", "WB: " };

inline bool isJ(string s) {
	return (s == "JR" || s == "BGTZ" || s == "J" || s == "JAL");
}

inline bool isLoad(string s) {
	return (s == "LW" || s == "LH" || s == "LHU" || s == "LB" || s == "LBU");
}

inline bool isBranchTo(Instruction inst) {
	return ((inst.name == "BEQ") || (inst.name == "BNE") || (inst.name == "BGTZ") || (inst.name == "J") || (inst.name == "JAL") || (inst.name == "JR"));
}

void nextStage() {
	Instruction inst = Global::IF_ID.inst;

	Global::Stall = false;
	Global::IF_ID.inst.fwdrs = false;
	Global::IF_ID.inst.fwdrt = false;
	Global::ID_EX.inst.fwdrs = false;
	Global::ID_EX.inst.fwdrt = false;
	Global::ID_EX.inst.fwdrs_EX_DM_from = false;
	Global::ID_EX.inst.fwdrt_EX_DM_from = false;
	int RsData, RtData;

	/*
	*	Forward in ID (only when branch)
	*/
	if (isBranchTo(inst)) {
		bool Branching = false;
		// Forward rs
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0) && (Global::EX_MEM.WriteDes == inst.rs)) {
			RsData = Global::EX_MEM.ALU_result;
			Global::IF_ID.inst.fwdrs = true;
		}
		else {
			Global::IF_ID.inst.fwdrs = false;
			RsData = Global::reg[inst.rs];
		}
		// Forward rt
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0) && (Global::EX_MEM.WriteDes == inst.rt)) {
			if (!isJ(inst.name)) {
				Global::IF_ID.inst.fwdrt = true;
				RtData = Global::EX_MEM.ALU_result;
			}
		}
		else {
			Global::IF_ID.inst.fwdrt = false;
			RtData = Global::reg[inst.rt];
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
		Global::Flush = (Branching) ? true : false;
	}
	else {
		Global::IF_ID.inst.fwdrs = false;
		Global::IF_ID.inst.fwdrt = false;
	}

	/*
	   Forwarding in EX (R_type forwarding from EX/MEM or MEM/WB)
	 */
	inst = Global::ID_EX.inst;
	if (!isBranchTo(inst)) {
		/*
		   Check for fwdrs
		 */
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0) && (Global::EX_MEM.WriteDes == inst.rs)) {
			if (inst.name != "SLL" && inst.name != "SRL" && inst.name != "SRA" && inst.name != "LUI")
			{
				Global::ID_EX.inst.fwdrs = true;
				Global::ID_EX.inst.fwdrs_EX_DM_from = false;
			}

		}

		else {
			Global::ID_EX.inst.fwdrs = false;
			Global::ID_EX.inst.fwdrs_EX_DM_from = false;
		}

		/*
		   Check for fwdrt
		 */
		if (!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes != 0))
		{
			if (inst.type == 'I')
			{
				if (inst.name == "SW" || inst.name == "SB" || inst.name == "SH")
				{
					if (Global::EX_MEM.WriteDes == inst.rt)
					{
						Global::ID_EX.inst.fwdrt = true;
						Global::ID_EX.inst.fwdrt_EX_DM_from = false;
					}
				}
			}
			else
			{
				if (Global::EX_MEM.WriteDes == inst.rt)
				{
					Global::ID_EX.inst.fwdrt = true;
					Global::ID_EX.inst.fwdrt_EX_DM_from = false;
				}
			}
		}
		if (Global::MEM_WB.RegWrite && (Global::MEM_WB.WriteDes != 0) && ((Global::MEM_WB.WriteDes == inst.rt) || (Global::MEM_WB.WriteDes == inst.rs)))
		{
			if (Global::MEM_WB.WriteDes == inst.rs)
			{
				if (inst.name != "SLL" && inst.name != "SRL" && inst.name != "SRA" && inst.name != "LUI")
				{
					if (!Global::ID_EX.inst.fwdrs)		//prevent double hazard
					{
						Global::ID_EX.inst.fwdrs = true;
						Global::ID_EX.inst.fwdrs_EX_DM_from = true;
					}
				}
			}
			if (Global::MEM_WB.WriteDes == inst.rt)
			{
				if (inst.type == 'I')
				{
					if (inst.name == "SW" || inst.name == "SB" || inst.name == "SH")
					{
						if (!Global::ID_EX.inst.fwdrt)		//prevent double hazard
						{
							Global::ID_EX.inst.fwdrt = true;
							Global::ID_EX.inst.fwdrt_EX_DM_from = true;
						}
					}
				}
				else
				{
					if (!Global::ID_EX.inst.fwdrt)		//prevent double hazard
					{
						Global::ID_EX.inst.fwdrt = true;
						Global::ID_EX.inst.fwdrt_EX_DM_from = true;
					}
				}
			}
		}
	}
	else {
		Global::ID_EX.inst.fwdrs = false;
		Global::ID_EX.inst.fwdrt = false;
		Global::ID_EX.inst.fwdrs_EX_DM_from = false;
		Global::ID_EX.inst.fwdrt_EX_DM_from = false;
	}

	// Stall Test
	inst = Global::IF_ID.inst;
	if (inst.name == "NOP" || inst.name == "HALT") return;
	if (!isBranchTo(inst)) {
		if (Global::ID_EX.MemRead && Global::ID_EX.WriteDes != 0 && ((Global::ID_EX.WriteDes == inst.rs) || (Global::ID_EX.WriteDes == inst.rt))) {
			if (Global::ID_EX.WriteDes == inst.rs) {
				{
					Global::Stall = true;
				}
			}
			else if (Global::ID_EX.WriteDes == inst.rt) {
				if (inst.type == 'I') {
					if (inst.name == "SW" || inst.name == "SH" || inst.name == "SB")
					{
						Global::Stall = true;
					}
				}
				else if (inst.type != 'I')
				{
					Global::Stall = true;
				}
			}
		}
	}
	else {
		if (Global::ID_EX.RegWrite && (Global::ID_EX.WriteDes != 0) && ((Global::ID_EX.WriteDes == inst.rs) || (Global::ID_EX.WriteDes == inst.rt))) {
			if (Global::ID_EX.WriteDes == inst.rs) {
				if (inst.name != "J" && inst.name != "JAL")
				{
					Global::Stall = true;
				}
			}
			if (Global::ID_EX.WriteDes == inst.rt) {
				if (!isJ(inst.name))
				{
					Global::Stall = true;
				}
			}
		}
		else if (Global::EX_MEM.MemRead && (Global::EX_MEM.WriteDes != 0) && ((Global::EX_MEM.WriteDes == inst.rs) || (Global::EX_MEM.WriteDes == inst.rt)))
		{
			Global::Stall = true;
		}
	}
}

void printCycle(ofstream &snapshot, int &Cycle) {
	File::snapshot << "cycle " << dec << Cycle++ << endl;

	for (int i = 0; i < 32; i++) {
		if (Global::reg[i] != lastThings::reg[i] || Cycle == 1)
		{
			File::snapshot << "$" << setw(2) << setfill('0') << dec << i;
			File::snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::reg[i] << endl;
		}
		lastThings::reg[i] = Global::reg[i];
	}
	if (Global::HI != lastThings::HI || Cycle == 1)
	{
		File::snapshot << "$" << setw(2) << "HI";
		File::snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::HI << endl;
	}
	lastThings::HI = Global::HI;
	if (Global::LO != lastThings::LO || Cycle == 1)
	{
		File::snapshot << "$" << setw(2) << "LO";
		File::snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::LO << endl;
	}
	lastThings::LO = Global::LO;
	File::snapshot << "PC: 0x" << setw(8) << setfill('0') << hex << uppercase << Global::PC << endl;
	File::snapshot << stagenames[0] << "0x" << setw(8) << setfill('0') << hex << uppercase << Global::Address[Global::PC];
	if (Global::Stall)
		File::snapshot << " to_be_stalled";
	else if (Global::Flush)
		File::snapshot << " to_be_flushed";
	File::snapshot << endl;
	if ((unsigned int)Global::Address[Global::PC] != 0xFFFFFFFF)
		Global::Halt = false;

	Buffer Buf;
	for (int i = 0; i < 4; i++) {
		if (i == 0) Buf = Global::IF_ID;
		else if (i == 1) Buf = Global::ID_EX;
		else if (i == 2) Buf = Global::EX_MEM;
		else Buf = Global::MEM_WB;

		if (Buf.inst.name != "HALT")
			Global::Halt = false;

		int op = ((unsigned int)Buf.inst.line) >> 26;
		int funct = ((unsigned int)(Buf.inst.line << 26)) >> 26;
		bool R_type = (op == 0 && funct == 0);
		File::snapshot << stagenames[i + 1];

		if (R_type && (Buf.inst.rt == 0) && (Buf.inst.rd == 0) && (Buf.inst.shamt == 0))
			File::snapshot << "NOP" << endl;

		else if (Global::Stall && i == 0)
			File::snapshot << Buf.inst.name << " to_be_stalled" << endl;

		else if (Buf.inst.fwdrs) {
			if (Buf.inst.fwdrs_EX_DM_from)
				File::snapshot << Buf.inst.name << " fwd_DM-WB_rs_$" << dec << Buf.inst.rs;
			else
				File::snapshot << Buf.inst.name << " fwd_EX-DM_rs_$" << dec << Buf.inst.rs;
			if (Buf.inst.fwdrt)
			{
				if (Buf.inst.fwdrt_EX_DM_from)
					File::snapshot << " fwd_DM-WB_rt_$" << dec << Buf.inst.rt;
				else
					File::snapshot << " fwd_EX-DM_rt_$" << dec << Buf.inst.rt;
			}
			File::snapshot << endl;
		}
		else if (Buf.inst.fwdrt) {
			if (Buf.inst.fwdrt_EX_DM_from)
				File::snapshot << Buf.inst.name << " fwd_DM-WB_rt_$" << dec << Buf.inst.rt;
			else
				File::snapshot << Buf.inst.name << " fwd_EX-DM_rt_$" << dec << Buf.inst.rt;
			File::snapshot << endl;
		}
		else
			File::snapshot << Buf.inst.name << endl;
	}
	File::snapshot << endl << endl;
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


void OpenFile(int &line,int &Cycle)
{
	char ch;
	File::snapshot = ofstream("snapshot.rpt", ios::out);
	 File::error_dump = ofstream("error_dump.rpt", ios::out);
	 File::image = ifstream("iimage.bin", ios::in | ios::binary);

	// PC
	for (int i = 0; i < 4; i++) {
		File::image.get(ch);
		line = (line << 8) | (unsigned char)ch;
	}
	Global::PC = line;
	// #Lines
	for (int i = 0; i < 4; i++) {
		File::image.get(ch);
		line = (line << 8) | (unsigned char)ch;
	}
	int Num = line;
	// Ins
	for (int i = 0; i < Num; i++) {
		for (int j = 0; j < 4; j++) {
			File::image.get(ch);
			line = (line << 8) | (unsigned char)ch;
		}
		Global::Address[Global::PC + i * 4] = line;
	}

	File::image.close();

	// dimage
	File::image.open("dimage.bin", ios::in | ios::binary);
	// $sp
	for (int i = 4; i > 0; i--) {
		File::image.get(ch);
		line = (line << 8) + (unsigned char)ch;
	}
	Global::reg[29] = line;
	// #Lines
	for (int i = 4; i > 0; i--) {
		File::image.get(ch);
		line = (line << 8) + (unsigned char)ch;
	}
	int index = line;
	for (int i = 0; i < index * 4; i++) {
		File::image.get(ch);
		Global::Memory[i] = ch;
	}
}
int main() {
	
	int line = 0, Cycle = 0;
	

	Initialize();
	OpenFile(line,Cycle);
	
	//Print 1st time
	printCycle(File::snapshot, Cycle);
	Global::Halt = false;
	Global::Stall = false;

	//Start Instructions
	while (!Global::Halt) {
		Global::Branch_taken = false;
		Global::Flush = false;
		for (int i = 0; i < 5; i++) Global::error_toggle[i] = false;
		WB();
		DM();
		EXE();
		ID();
		IF();
		// Print errors
		for (int i = 0; i < 5; i++) {
			if (Global::error_toggle[i])
				File::error_dump << "In cycle " << Cycle << errorMsg[i] << endl;
		}

		// Halt if misaligned or addovf
		if (Global::error_toggle[1] || Global::error_toggle[2]) break;

		// Branch taken then flush
		if (!Global::Stall && Global::Branch_taken) {
			Instruction NOP;
			Global::PC = Global::Branch_PC;
			Global::IF_ID.inst = NOP;
		}

		else if (!Global::Stall)
			Global::PC += 4;

		nextStage();
		printCycle(File::snapshot, Cycle);

		if (Global::Halt == true)
			break;

	}

	File::snapshot.close();
	File::error_dump.close();
	return 0;
}
