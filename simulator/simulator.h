#ifndef simulator_h
#define simulator_h
#pragma warning( disable : 4996 )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define MEM_MAX_SIZE 1024
#define R	  0
#define ADD   32
#define ADDU  33
#define SUB   34
#define AND   36
#define OR    37
#define XOR   38
#define NOR   39
#define NAND  40
#define SLT   42
#define SLL    0
#define SRL    2
#define SRA    3
#define JR     8
#define MULT   24
#define MULTU  25
#define MFHI   16
#define MFLO   18

#define ADDI   8
#define ADDIU  9
#define LW    35
#define LH    33
#define LHU   37
#define LB    32
#define LBU   36
#define SW    43
#define SH    41
#define SB    40
#define LUI   15
#define ANDI  12
#define ORI   13
#define NORI  14
#define SLTI  10
#define BEQ    4
#define BNE    5
#define BGTZ   7

#define J      2
#define JAL    3
#define HALT  63

char* inst[100];
char* rInst[100];

char *iimageBuf, *dimageBuf;
FILE *iimage, *dimage, *error_dump, *snapshot;

int writeRegZero, numOverflow, HILOOverWrite, memOverflow, dataMisaligned;
int toggledHILO, toggledMULT;
int IF_HALT, ID_HALT, EX_HALT, DM_HALT, WB_HALT,ERROR_HALT;
int RS_TO_EX, RT_TO_EX, RS_TO_ID, RT_TO_ID;

typedef struct _Buffer {
	unsigned inst_in, inst_out;
	unsigned pc_plus_four_in, pc_plus_four_out;
	unsigned opcode_in, opcode_out;
	unsigned funct_in, funct_out;
	unsigned shamt_in, shamt_out;
	unsigned rt_in, rt_out;
	unsigned rd_in, rd_out;
	unsigned rs_in, rs_out;

	unsigned alu_src_in, alu_src_out;

	unsigned num_rs_in, num_rs_out;
	unsigned num_rt_in, num_rt_out;
	unsigned extended_imme_in, extended_imme_out;

	int pc_branch_in, pc_branch_out;

	unsigned pc_in, pc_out;
	unsigned reg_to_write_in, reg_to_write_out;

	unsigned write_data_in, write_data_out;
	unsigned alu_result_in, alu_result_out;
	unsigned read_data_in, read_data_out;

}Buffer;

int STALL;

Buffer IF_ID;
Buffer ID_EX;
Buffer EX_DM;
Buffer DM_WB;


 unsigned char iMemory[1024];
 unsigned iPos, address;
 unsigned op, rs, rt, rd, func, shamt, immediate;

 unsigned char dMemory[1024];
 unsigned dAddr, dPos;

 unsigned reg[32],lastreg[32], PC, HI,lastHI, LO,lastLO;

 unsigned cycle;

void OpenFile();
void iimageParser();
void dimageParser();
void errorDump();
void initialize();
void printParsed();
void snapShot_Reg();
void snapShot_Buffer();
void instToString();
#endif