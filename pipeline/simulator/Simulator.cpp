#include "Simulator.h"

int lastThings::reg[32],lastThings::LO,lastThings::HI;
int Simulator::Address[1024];
int Simulator::Memory[1024];
int Simulator::reg[32], Simulator::PC, Simulator::Branch_PC,Simulator::LO,Simulator::HI;
bool Simulator::Halt, Simulator::Stall, Simulator::Flush,Simulator::toggle_HILO,Simulator::toggle_MULT;
bool Simulator::Branch_taken;
bool Simulator::error_toggle[5];
Buffer Simulator::IF_ID, Simulator::ID_EX, Simulator::EX_MEM, Simulator::MEM_WB ,Simulator::WB_AFTER;
ofstream File::snapshot;
ofstream File::error_dump;
ifstream File::image;
void Simulator::debug()
{
	std::cout << "IF_ID.ALU.result : " << IF_ID.ALU_result << '\n';
	std::cout << "ID_EX.ALU.result : " << ID_EX.ALU_result << '\n';
	std::cout << "EX_MEM.ALU.result : " << EX_MEM.ALU_result << '\n';
}
bool Simulator::isJ(string s) {
	return (s == "JR" || s == "BGTZ" || s == "J" || s == "JAL");
}

bool Simulator::isLoad(string s) {
	return (s == "LW" || s == "LH" || s == "LHU" || s == "LB" || s == "LBU");
}

bool Simulator::isBranch(Instruction inst) {
	return ((inst.name == "BEQ") || (inst.name == "BNE") || (inst.name == "BGTZ") || (inst.name == "J") || (inst.name == "JAL") || (inst.name == "JR"));
}

bool Simulator::notS(string s) {
	return (s == "NOP") || (s == "SW") || (s == "SB") || (s == "SH");
}
bool Simulator::isHILO(Instruction inst) {
	return ( (inst.name == "MFHI") || (inst.name == "MFLO"));
}
bool Simulator::isShift(Instruction inst) {
	return ((inst.name == "SRA") || (inst.name == "SRL")||(inst.name=="SLL"));
}