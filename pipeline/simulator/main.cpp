#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>


#include "Simulator.h"
#include "pipeline.h"

using namespace std;

string errorMsg[5] = { ": Write $0 Error", ": Address Overflow", ": Misalignment Error", ": Number Overflow",": Overwrite HI-LO registers" };
string stagenames[5] = { "IF: ", "ID: ", "EX: ", "DM: ", "WB: " };




void printCycle(ofstream &snapshot, int &Cycle) {
	File::snapshot << "cycle " << dec << Cycle++ << endl;

	for (int i = 0; i < 32; i++) {
		if (Simulator::reg[i] != lastThings::reg[i] || Cycle == 1)
		{
			File::snapshot << "$" << setw(2) << setfill('0') << dec << i;
			File::snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Simulator::reg[i] << endl;
		}
		lastThings::reg[i] = Simulator::reg[i];
	}
	if (Simulator::HI != lastThings::HI || Cycle == 1)
	{
		File::snapshot << "$" << setw(2) << "HI";
		File::snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Simulator::HI << endl;
	}
	lastThings::HI = Simulator::HI;
	if (Simulator::LO != lastThings::LO || Cycle == 1)
	{
		File::snapshot << "$" << setw(2) << "LO";
		File::snapshot << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Simulator::LO << endl;
	}
	lastThings::LO = Simulator::LO;
	File::snapshot << "PC: 0x" << setw(8) << setfill('0') << hex << uppercase << Simulator::PC << endl;
	File::snapshot << stagenames[0] << "0x" << setw(8) << setfill('0') << hex << uppercase << Simulator::Address[Simulator::PC];
	if (Simulator::Stall)
		File::snapshot << " to_be_stalled";
	else if (Simulator::Flush)
		File::snapshot << " to_be_flushed";
	File::snapshot << endl;
	if ((unsigned int)Simulator::Address[Simulator::PC] != 0xFFFFFFFF)
		Simulator::Halt = false;

	Buffer Buf;
	for (int i = 0; i < 4; i++) {
		if (i == 0) Buf = Simulator::IF_ID;
		else if (i == 1) Buf = Simulator::ID_EX;
		else if (i == 2) Buf = Simulator::EX_MEM;
		else Buf = Simulator::MEM_WB;

		if (Buf.inst.name != "HALT")
			Simulator::Halt = false;

		int op = ((unsigned int)Buf.inst.line) >> 26;
		int funct = ((unsigned int)(Buf.inst.line << 26)) >> 26;
		bool R_type = (op == 0 && funct == 0);
		File::snapshot << stagenames[i + 1];

		if (R_type && (Buf.inst.rt == 0) && (Buf.inst.rd == 0) && (Buf.inst.shamt == 0))
			File::snapshot << "NOP" << endl;

		else if (Simulator::Stall && i == 0)
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
		Simulator::reg[i] = 0;
	for (int i = 0; i < 32; i++)
		lastThings::reg[i] = 0;
	for (int i = 0; i < 1024; i++)
		Simulator::Address[i] = 0;
	Simulator::LO = 0; Simulator::HI = 0;
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
	Simulator::PC = line;
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
		Simulator::Address[Simulator::PC + i * 4] = line;
	}

	File::image.close();

	// dimage
	File::image.open("dimage.bin", ios::in | ios::binary);
	// $sp
	for (int i = 4; i > 0; i--) {
		File::image.get(ch);
		line = (line << 8) + (unsigned char)ch;
	}
	Simulator::reg[29] = line;
	// #Lines
	for (int i = 4; i > 0; i--) {
		File::image.get(ch);
		line = (line << 8) + (unsigned char)ch;
	}
	int index = line;
	for (int i = 0; i < index * 4; i++) {
		File::image.get(ch);
		Simulator::Memory[i] = ch;
	}
}
int main() {
	
	int line = 0, Cycle = 0;
	Initialize();
	OpenFile(line,Cycle);

	//Print 1st time
	printCycle(File::snapshot, Cycle);
	Simulator::Halt = false;
	Simulator::Stall = false;

	//Start Instructions
	while (!Simulator::Halt) {
		Simulator::Branch_taken = false;
		Simulator::Flush = false;
		for (int i = 0; i < 5; i++) Simulator::error_toggle[i] = false;
		WB();

		DM();
		
		EXE();
		
		ID();

		IF();
		// Print errors
		for (int i = 0; i < 5; i++) {
			if (Simulator::error_toggle[i])
				File::error_dump << "In cycle " << Cycle << errorMsg[i] << endl;
		}

		// Halt if misaligned or addovf
		if (Simulator::error_toggle[1] || Simulator::error_toggle[2]) break;

		// Branch taken then flush
		if (!Simulator::Stall && Simulator::Branch_taken) {
			Instruction NOP;
			Simulator::PC = Simulator::Branch_PC;
			Simulator::IF_ID.inst = NOP;
		}

		else if (!Simulator::Stall)
			Simulator::PC += 4;

		nextStage();
		printCycle(File::snapshot, Cycle);

		if (Simulator::Halt == true)
			break;

	}

	File::snapshot.close();
	File::error_dump.close();
	return 0;
}
