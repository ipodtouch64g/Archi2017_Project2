#include "Simulator.h"

int lastThings::reg[32],lastThings::LO,lastThings::HI;
int Global::Address[1024];
int Global::Memory[1024];
int Global::reg[32], Global::PC, Global::Branch_PC,Global::LO,Global::HI;
bool Global::Halt, Global::Stall, Global::Flush,Global::toggle_HILO,Global::toggle_MULT;
bool Global::Branch_taken;
bool Global::error_toggle[5];
Buffer Global::IF_ID, Global::ID_EX, Global::EX_MEM, Global::MEM_WB ,Global::WB_AFTER;
ofstream File::snapshot;
ofstream File::error_dump;
ifstream File::image;
void Global::debug()
{
	std::cout << "IF_ID.ALU.result : " << IF_ID.ALU_result << endl;
	std::cout << "ID_EX.ALU.result : " << ID_EX.ALU_result << endl;
	std::cout << "EX_MEM.ALU.result : " << EX_MEM.ALU_result << endl;
}
