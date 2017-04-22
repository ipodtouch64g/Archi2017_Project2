#include "GlobalVar.h"

int lastThings::reg[32],lastThings::LO,lastThings::HI;
int Global::Address[1024];
map< int,char > Global::Memory;
int Global::reg[32], Global::PC, Global::Branch_PC,Global::LO,Global::HI;
bool Global::Halt, Global::Stall, Global::Flush,Global::toggle_HILO,Global::toggle_MULT;
bool Global::Branch_taken;
bool Global::error_type[5];
Buffer Global::IF_ID, Global::ID_EX, Global::EX_MEM, Global::MEM_WB ,Global::WB_AFTER;

void Global::debug()
{
	std::cout << "IF_ID.ALU.result : " << IF_ID.ALU_result << endl;
	std::cout << "ID_EX.ALU.result : " << ID_EX.ALU_result << endl;
	std::cout << "EX_MEM.ALU.result : " << EX_MEM.ALU_result << endl;
}
