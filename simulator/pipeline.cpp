#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include "GlobalVar.h"
#include "Stage.h"

using namespace std;

string Error_Message[5] = {": Write $0 Error", ": Address Overflow", ": Misalignment Error", ": Number Overflow",": Overwrite HI-LO registers"};
string fiveStage[5] = {"IF: ", "ID: ", "EX: ", "DM: ", "WB: "};
Instruction fiveStageIns[5];

bool noRS(string s){
								return (s=="SLL" || s=="SRL" || s=="SRA" || s=="LUI");
}

bool isJ(string s){
								return (s=="JR" || s=="BGTZ" || s=="J" || s=="JAL");
}

bool isLoad(string s){
								return (s=="LW" || s=="LH" || s=="LHU" || s=="LB" || s=="LBU");
}

bool isBranch2(Instruction ins){
								return ((ins.Name=="BEQ") || (ins.Name=="BNE") || (ins.Name=="BGTZ") || (ins.Name=="J") || (ins.Name=="JAL") || (ins.Name=="JR"));
}

void NextStageTest(){
								Instruction ins = Global::IF_ID.ins;

								Global::Stall = false;
								Global::IF_ID.ins.fwdrs = false;
								Global::IF_ID.ins.fwdrt = false;
								Global::ID_EX.ins.fwdrs = false;
								Global::ID_EX.ins.fwdrt = false;
								Global::ID_EX.ins.fwd_EX_DM_from = false;
								int RsData, RtData;

								// Forwarding in ID
								if(isBranch2(ins)) {
																bool NextBranch = false;
																// Forward rs
																if(!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes!=0) && (Global::EX_MEM.WriteDes == ins.rs)) {
																								Global::IF_ID.ins.fwdrs = true;
																								RsData = Global::EX_MEM.ALU_result;
																}
																else{
																								Global::IF_ID.ins.fwdrs = false;
																								if(Global::MEM_WB.RegWrite && (Global::MEM_WB.ins.Name!="NOP") && (Global::MEM_WB.WriteDes == ins.rs) && Global::MEM_WB.WriteDes!=0) {
																																if(Global::MEM_WB.ins.type == 'R')
																																								RsData = Global::MEM_WB.ALU_result;
																																else if(Global::MEM_WB.ins.type == 'I')
																																								RsData = Global::MEM_WB.Data;
																								}
																								else
																																RsData = Global::reg[ins.rs];
																}
																// Forward rt
																if(!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes!=0) && (Global::EX_MEM.WriteDes == ins.rt)) {
																								if(!isJ(ins.Name)) {
																																Global::IF_ID.ins.fwdrt = true;
																																RtData = Global::EX_MEM.ALU_result;
																								}
																}
																else{
																								Global::IF_ID.ins.fwdrt = false;
																								if(Global::MEM_WB.RegWrite && Global::MEM_WB.WriteDes == ins.rt && Global::MEM_WB.WriteDes!=0) {
																																if(Global::MEM_WB.ins.type == 'R')
																																								RtData = Global::MEM_WB.ALU_result;
																																else if(Global::MEM_WB.ins.type == 'I')
																																								RtData = Global::MEM_WB.Data;
																								}
																								else
																																RtData = Global::reg[ins.rt];
																}
																switch(ins.opcode) {
																case 4: // beq
																								NextBranch = (RsData == RtData);
																								break;
																case 5: // bne
																								NextBranch = (RsData != RtData);
																								break;
																case 7: // bgtz
																								NextBranch = (RsData > 0);
																								break;
																case 2: // j
																								NextBranch = true;
																								break;
																case 3: // jal
																								NextBranch = true;
																								break;
																case 0: // jg
																								NextBranch = true;
																}
																Global::Flush = (NextBranch) ? true : false;
								}
								else{
																Global::IF_ID.ins.fwdrs = false;
																Global::IF_ID.ins.fwdrt = false;
								}
								/*
								   Forwarding in EX (check forwarding from EX/MEM or MEM/WB)
								 */
								ins = Global::ID_EX.ins;
								if(!isBranch2(ins)) {
																/*
																   Check for fwdrs
																 */
																if(!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes!=0) && (Global::EX_MEM.WriteDes == ins.rs)) {
																								if(ins.Name!="SLL" && ins.Name!="SRL" && ins.Name!="SRA" && ins.Name!="LUI")
																								{
																																Global::ID_EX.ins.fwdrs = true;
																																Global::ID_EX.ins.fwd_EX_DM_from = false;
																								}

																}

																else{
																								Global::ID_EX.ins.fwdrs = false;
																								Global::ID_EX.ins.fwd_EX_DM_from = false;
																}
																/*
																   Check for fwdrt
																 */
																if(!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && (Global::EX_MEM.WriteDes!=0))
																{
																								if(ins.type=='I')
																								{
																																if(ins.Name=="SW" || ins.Name=="SB" || ins.Name=="SH")
																																{
																																								if(Global::EX_MEM.WriteDes == ins.rt)
																																								{
																																																Global::ID_EX.ins.fwdrt = true;
																																																Global::ID_EX.ins.fwd_EX_DM_from = false;
																																								}

																																}
																								}
																								else
																								{
																																if(Global::EX_MEM.WriteDes == ins.rt)
																																{

																																								cout<<"mult ins rt\n";
																																								Global::ID_EX.ins.fwdrt = true;
																																								Global::ID_EX.ins.fwd_EX_DM_from = false;
																																}

																								}
																}
																else if(Global::MEM_WB.MemRead && Global::MEM_WB.RegWrite && (Global::MEM_WB.WriteDes!=0) && ((Global::MEM_WB.WriteDes == ins.rt)||(Global::MEM_WB.WriteDes == ins.rs)))
																{
																								cout<<"lw forward\n";
																								if(Global::MEM_WB.WriteDes == ins.rs)
																								{
																																Global::ID_EX.ins.fwdrs = true;
																																Global::ID_EX.ins.fwd_EX_DM_from = true;

																								}
																								else
																								{
																																Global::ID_EX.ins.fwdrt = true;
																																Global::ID_EX.ins.fwd_EX_DM_from = true;
																								}


																}
																else
																{
																								Global::ID_EX.ins.fwdrt = false;
																								Global::ID_EX.ins.fwd_EX_DM_from = false;
																}

								}
								else{
																Global::ID_EX.ins.fwdrs = false;
																Global::ID_EX.ins.fwdrt = false;
																Global::ID_EX.ins.fwd_EX_DM_from = false;
								}


								// Stall Test
								ins = Global::IF_ID.ins;
								if(ins.Name=="NOP" || ins.Name=="HALT") return;
								if(!isBranch2(ins)) {
																// if(Global::EX_MEM.MemRead && Global::EX_MEM.WriteDes!=0 && ((Global::EX_MEM.WriteDes == ins.rs) || (Global::EX_MEM.WriteDes == ins.rt))) {
																//         if(Global::EX_MEM.WriteDes == ins.rs) {
																//                 if(!(Global::ID_EX.RegWrite && Global::ID_EX.WriteDes!=0 && Global::ID_EX.WriteDes==ins.rs))
																//                 {
																//                  cout<<"EX_MEM.WriteRes : "<<Global::EX_MEM.WriteDes<<endl;
																//
																//                  cout<<"1\n";
																//                  Global::Stall = true;
																//                 }
																//
																//         }
																//         else if(Global::EX_MEM.WriteDes==ins.rt) {
																//                 if(!(Global::ID_EX.RegWrite && Global::ID_EX.WriteDes!=0 && Global::ID_EX.WriteDes==ins.rt)) {
																//                         if(ins.type=='I') {
																//                                 if(ins.Name=="SW" || ins.Name=="SH" || ins.Name=="SB")
																//                                 {
																//                                  cout<<"2\n";
																//                                  Global::Stall = true;
																//                                 }
																//                         }
																//                         else
																//                         {
																//                          cout<<"3\n";
																//                          Global::Stall = true;
																//                         }
																//                 }
																//         }
																// }
																if(Global::ID_EX.MemRead && Global::ID_EX.WriteDes!=0 && ((Global::ID_EX.WriteDes == ins.rs) || (Global::ID_EX.WriteDes == ins.rt))) {
																								if(Global::ID_EX.WriteDes == ins.rs) {
																																{
																																								cout<<"4\n";
																																								Global::Stall = true;
																																}
																								}
																								else if(Global::ID_EX.WriteDes == ins.rt) {
																																if(ins.type=='I') {
																																								if(ins.Name=="SW" || ins.Name=="SH" || ins.Name=="SB")
																																								{
																																																cout<<"5\n";
																																																Global::Stall = true;
																																								}
																																}
																																else if(ins.type!='I')
																																{
																																								cout<<"6\n";
																																								Global::Stall = true;
																																}
																								}
																}
																if(!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && Global::EX_MEM.WriteDes!=0 && Global::EX_MEM.WriteDes==ins.rs && !noRS(ins.Name)) {
																								if(Global::ID_EX.RegWrite) {
																																if(Global::ID_EX.WriteDes!=ins.rs)
																																{
																																								cout<<"7\n";
																																								Global::Stall = true;
																																}
																								}
																								else
																								{
																																cout<<"8\n";
																																Global::Stall = true;
																								}
																}
																if(!Global::EX_MEM.MemRead && Global::EX_MEM.RegWrite && Global::EX_MEM.WriteDes!=0 && Global::EX_MEM.WriteDes==ins.rt && !isLoad(ins.Name)) {
																								if(Global::ID_EX.RegWrite) {
																																if(Global::ID_EX.WriteDes!=ins.rt) {
																																								if(ins.type!='I')
																																								{
																																																cout<<"9\n";
																																																Global::Stall = true;
																																								}
																																								else{
																																																if(ins.Name=="SW" || ins.Name=="SH" || ins.Name=="SB")
																																																{
																																																								cout<<"10\n";
																																																								Global::Stall = true;
																																																}
																																								}
																																}
																								}
																								else if(ins.type!='I' || ins.Name=="SW" || ins.Name=="SH" || ins.Name=="SB")
																								{
																																cout<<"11\n";
																																Global::Stall = true;
																								}
																}
								}
								else{
																if(Global::ID_EX.RegWrite && (Global::ID_EX.WriteDes!=0) && ((Global::ID_EX.WriteDes == ins.rs) || (Global::ID_EX.WriteDes == ins.rt))) {
																								if(Global::ID_EX.WriteDes == ins.rs) {
																																if(ins.Name!="J" && ins.Name!="JAL")
																																{
																																								cout<<"12\n";
																																								Global::Stall = true;
																																}
																								}
																								if(Global::ID_EX.WriteDes == ins.rt) {
																																if(!isJ(ins.Name))
																																{
																																								cout<<"13\n";
																																								Global::Stall = true;
																																}
																								}
																}
																else if(Global::EX_MEM.MemRead && (Global::EX_MEM.WriteDes!=0) && ((Global::EX_MEM.WriteDes == ins.rs) || (Global::EX_MEM.WriteDes == ins.rt)))
																{
																								cout<<"14\n";
																								Global::Stall = true;
																}
								}
}

void Initialize(){
								for(int i=0; i<32; i++)
																Global::reg[i] = 0;
								for(int i=0; i<32; i++)
																lastThings::reg[i] = 0;
								for(int i=0; i<1024; i++)
																Global::Address[i] = 0;
								Global::LO =0; Global::HI=0;
								lastThings::LO=0; lastThings::HI=0;
}

void cyclePrint(ofstream &fout, int &Cycle){
								fout << "cycle " << dec << Cycle++ << endl;
								for(int i=0; i<32; i++) {
																if(Global::reg[i]!=lastThings::reg[i] || Cycle==1)
																{
																								fout << "$" << setw(2) << setfill('0') << dec << i;
																								fout << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::reg[i] << endl;
																}
																lastThings::reg[i]=Global::reg[i];
								}
								if(Global::HI!=lastThings::HI || Cycle==1)
								{
																fout << "$" << setw(2) << "HI";
																fout << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::HI << endl;
								}
								lastThings::HI=Global::HI;

								if(Global::LO!=lastThings::LO || Cycle==1)
								{
																fout << "$" << setw(2) << "LO";
																fout << ": 0x" << setw(8) << setfill('0') << hex << uppercase << Global::LO<< endl;
								}
								lastThings::LO=Global::LO;
								fout << "PC: 0x" << setw(8) << setfill('0') << hex << uppercase << Global::PC << endl;
								fout << fiveStage[0] << "0x" << setw(8) << setfill('0') << hex << uppercase << Global::Address[Global::PC];
								if(Global::Stall)
																fout << " to_be_stalled";
								else if(Global::Flush)
																fout << " to_be_flushed";
								fout << endl;
								if((unsigned int)Global::Address[Global::PC] != 0xFFFFFFFF)
																Global::Halt = false;
								Buffer B;
								for(int i=0; i<4; i++) {
																if(i == 0) B = Global::IF_ID;
																else if(i == 1) B = Global::ID_EX;
																else if(i == 2) B = Global::EX_MEM;
																else B = Global::MEM_WB;

																if(B.ins.Name != "HALT")
																								Global::Halt = false;
																int op = ((unsigned int)B.ins.Word) >> 26;
																int fu = ((unsigned int)(B.ins.Word << 26)) >> 26;
																bool check = (op==0 && fu==0);
																fout << fiveStage[i+1];
																if(check && (B.ins.rt == 0) && (B.ins.rd == 0) && (B.ins.shamt == 0))
																								fout << "NOP" << endl;
																else if(Global::Stall && i == 0)
																								fout << B.ins.Name << " to_be_stalled" << endl;
																else if(B.ins.fwdrs) {
																								if(B.ins.fwd_EX_DM_from)
																																fout << B.ins.Name << " fwd_DM-WB_rs_$" << dec << B.ins.rs;
																								else
																																fout << B.ins.Name << " fwd_EX-DM_rs_$" << dec << B.ins.rs;
																								if(B.ins.fwdrt)
																								{
																																if(B.ins.fwd_EX_DM_from)
																																								fout  << " fwd_DM-WB_rt_$" << dec << B.ins.rt;
																																else
																																								fout  << " fwd_EX-DM_rt_$" << dec << B.ins.rt;
																								}

																								fout << endl;
																}
																else if(B.ins.fwdrt) {
																								if(B.ins.fwd_EX_DM_from)
																																fout << B.ins.Name << " fwd_DM-WB_rt_$" << dec << B.ins.rt;
																								else
																																fout << B.ins.Name << " fwd_EX-DM_rt_$" << dec << B.ins.rt;
																								fout << endl;
																}

																else
																								fout << B.ins.Name << endl;
								}
								fout << endl << endl;
}

int main(){
								char ch;
								int Word = 0, Cycle = 0;

								// Initialize register;
								Initialize();

								ofstream fout("snapshot.rpt", ios::out);
								ofstream Errorout("error_dump.rpt", ios::out);

								// Read iimage.bin
								ifstream fin("iimage.bin", ios::in | ios::binary);
								if(!fin) {
																cout << "Error to load 'iimage.bin'!\n";
																return 0;
								}
								// Read PC
								for(int i=0; i<4; i++) {
																fin.get(ch);
																Word = (Word << 8) | (unsigned char)ch;
								}
								Global::PC = Word;
								// Read numbers of words
								for(int i=0; i<4; i++) {
																fin.get(ch);
																Word = (Word << 8) | (unsigned char)ch;
								}
								int Num = Word;
								// Read Instructions
								for(int i=0; i<Num; i++) {
																for(int j=0; j<4; j++) {
																								fin.get(ch);
																								Word = (Word << 8) | (unsigned char)ch;
																}
																Global::Address[Global::PC+i*4] = Word;
								}

								fin.close();

								// Read dimage.bin
								fin.open("dimage.bin", ios::in | ios::binary);
								// Read $sp
								for(int i=4; i>0; i--) {
																fin.get(ch);
																Word = (Word << 8) + (unsigned char)ch;
								}
								Global::reg[29] = Word;
								// Numbers of words
								for(int i=4; i>0; i--) {
																fin.get(ch);
																Word = (Word << 8) + (unsigned char)ch;
								}
								int NumbersOfWords = Word;
								for(int i=0; i<NumbersOfWords*4; i++) {
																fin.get(ch);
																Global::Memory[i] = ch;
								}

								cyclePrint(fout, Cycle);
								Global::Halt = false;
								Global::Stall = false;

								//Start Instructions
								while(!Global::Halt) {
																Global::Branch_taken = false;
																Global::Flush = false;
																for(int i=0; i<4; i++) Global::error_type[i] = false;
																Write_Back();
																Memory_Access();
																Execute();
																Instruction_Decode();
																Instruction_Fetch();
																for(int i=0; i<5; i++) {
																								if(Global::error_type[i]==true)
																																Errorout << "In cycle " << Cycle << Error_Message[i] << endl;
																}
																// Error Halt
																if(Global::error_type[1] || Global::error_type[2]) break;

																if(!Global::Stall && Global::Branch_taken) {
																								Instruction NOP;
																								Global::PC = Global::Branch_PC;
																								Global::IF_ID.ins = NOP;
																}
																else if(!Global::Stall)
																								Global::PC += 4;
																NextStageTest();
																cyclePrint(fout, Cycle);
																if(Global::Halt==true)
																								break;
								}
								fout.close();
								Errorout.close();
								return 0;
}
