#include "GlobalVar.h"

int lastThings::reg[32],lastThings::LO,lastThings::HI;
int Global::Address[1024];
map< int,char > Global::Memory;
int Global::reg[32], Global::PC, Global::Branch_PC,Global::LO,Global::HI;
bool Global::Halt, Global::Stall, Global::Flush,Global::toggle_HILO,Global::toggle_MULT;
bool Global::Branch_taken;
bool Global::error_type[5];
Buffer Global::IF_ID, Global::ID_EX, Global::EX_MEM, Global::MEM_WB;
