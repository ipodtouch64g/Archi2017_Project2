#include "simulator.h"



// Open image files and output files
void OpenFile()
{

	unsigned len_iimage = 0, len_dimage = 0;

	error_dump=fopen("error_dump.rpt", "w");
	snapshot= fopen("snapshot.rpt", "w");
	iimage=fopen("iimage.bin", "rb");
	dimage=fopen("dimage.bin", "rb");


	fseek(iimage, 0, SEEK_END);
	fseek(dimage, 0, SEEK_END);

	len_iimage = (unsigned)ftell(iimage);
	len_dimage = (unsigned)ftell(dimage);

	rewind(iimage);
	rewind(dimage);

	iimageBuf = (char*)malloc(len_iimage * sizeof(char));
	dimageBuf = (char*)malloc(len_dimage * sizeof(char));

	//Finally read it into buffer
	fread(iimageBuf, 1, len_iimage, iimage);
	fread(dimageBuf, 1, len_dimage, dimage);

	fclose(iimage);
	fclose(dimage);
	iimageParser();
	dimageParser();
	//printParsed();
}
//Deal with iimage
void iimageParser()
{
	unsigned tmp = 0, lineNum = 0;

	//First 4 bytes is initial value of PC
	for (int i = 0; i<4; i++)
		tmp = (tmp << 8) + (unsigned char)iimageBuf[i];
	PC = tmp;
	IF_ID.pc_plus_four_out = tmp;
	//Second 4 bytes is # of how many lines of instructions below
	for (int i = 4; i<8; i++)
		lineNum = (lineNum << 8) + (unsigned char)iimageBuf[i];

	//Others are instructions,write them to iMemory
	for (int i = 8; i<8 + lineNum * 4; i++)
		iMemory[tmp++] = iimageBuf[i];
}
//Deal with dimage
void dimageParser()
{
	unsigned tmp = 0, lineNum = 0, index = 0;

	//First 4 bytes is initial value of SP
	for (int i = 0; i<4; i++)
		tmp = (tmp << 8) + (unsigned char)dimageBuf[i];
	reg[29] = tmp;

	//Second 4 bytes is # of how many lines of instructions below
	for (int i = 4; i<8; i++)
		lineNum = (lineNum << 8) + (unsigned char)dimageBuf[i];

	//Others are instructions,write them to dMemory
	for (int i = 8; i<8 + lineNum * 4; i++)
		dMemory[index++] = dimageBuf[i];
}
//Debug
void printParsed()
{
	printf("PC: 0x%08X\n", PC);
	printf("HI: 0x%08X\n", HI);
	printf("LO: 0x%08X\n", LO);
	for (int i = 0; i<1024; i += 4)
		printf("iMemory at 0x%08X : 0x%02X%02X%02X%02X\n", i, iMemory[i], iMemory[i + 1], iMemory[i + 2], iMemory[i + 3]);
	for (int i = 0; i<1024; i += 4)
		printf("dMemory at 0x%08X : 0x%02X%02X%02X%02X\n", i, dMemory[i], dMemory[i + 1], dMemory[i + 2], dMemory[i + 3]);
}
//Dump errors
void errorDump()
{
	if (writeRegZero)
	{
		fprintf(error_dump, "In cycle %d: Write $0 Error\n", cycle);
		//printf("In cycle %d: Write $0 Error\n", cycle);
	}


	if (numOverflow)
	{
		fprintf(error_dump, "In cycle %d: Number Overflow\n", cycle);
		//printf("In cycle %d: Number Overflow\n", cycle);
	}


	if (HILOOverWrite)
	{
		fprintf(error_dump, "In cycle %d: Overwrite HI-LO registers\n", cycle);
		//printf("In cycle %d: Overwrite HI-LO registers\n", cycle);
	}


	if (memOverflow)
	{
		fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
		//printf("In cycle %d: Address Overflow\n", cycle);
	}

	if (dataMisaligned)
	{
		fprintf(error_dump, "In cycle %d: Misalignment Error\n", cycle);
		//printf("In cycle %d: Misalignment Error\n", cycle);
	}

}
//Initialize at first
void initialize() {
	memset(&IF_ID, 0, sizeof(IF_ID));
	memset(&ID_EX, 0, sizeof(ID_EX));
	memset(&EX_DM, 0, sizeof(EX_DM));
	memset(&DM_WB, 0, sizeof(DM_WB));
	memset(&reg, 0, sizeof(reg));
	memset(&lastreg, 0, sizeof(lastreg));
	PC = 0; 
	LO = 0; lastLO = 0;
	HI = 0; lastHI = 0;
	IF_HALT = 0;
	ID_HALT = 0;
	EX_HALT = 0;
	DM_HALT = 0;
	WB_HALT = 0;
	ERROR_HALT = 0;
	writeRegZero = 0;
	numOverflow = 0; 
	HILOOverWrite = 0; 
	memOverflow = 0; 
	dataMisaligned = 0;
	toggledHILO = 0; 
	toggledMULT=0;

	instToString();
}
//Snapshot of reg
void snapShot_Reg() {
	fprintf(snapshot, "cycle %d\n", cycle++);
	for (int i = 0; i<32; i++) {
		if (lastreg[i] != reg[i] || cycle==1)
		{
			fprintf(snapshot, "$%02d: 0x", i);
			fprintf(snapshot, "%08X\n", reg[i]);
		}
		lastreg[i] = reg[i];
	}
	if (HI != lastHI || cycle == 1) {
		fprintf(snapshot, "$HI: 0x");
		fprintf(snapshot, "%08X\n", HI);
	}
		
		if (LO != lastLO || cycle == 1) {
			fprintf(snapshot, "$LO: 0x");
			fprintf(snapshot, "%08X\n", LO);
	}
		
	lastHI = HI; lastLO = LO;
	if (ID_EX.pc_branch_out) {
		fprintf(snapshot, "PC: 0x%08X\n", ID_EX.pc_out);
		fprintf(snapshot, "IF: 0x");
		for (int i = 0; i < 4; i++) {
			fprintf(snapshot, "%02X",iMemory[ID_EX.pc_out + i] & 0xff);
		}
	}

	else {
		fprintf(snapshot, "PC: 0x%08X\n", PC);
		fprintf(snapshot, "IF: 0x");
		for (int i = 0; i < 4; i++) {
			fprintf(snapshot, "%02X", iMemory[PC + i] & 0xff);
		}
	}
}
//Snapshot of buffer(stages)
void snapShot_Buffer() {
	int isNOP = 0;
	

	if (STALL) {
		fprintf(snapshot, " to_be_stalled");
	}
	else if (ID_EX.pc_branch_in == 1) {
		fprintf(snapshot, " to_be_flushed");
	}
	fprintf(snapshot, "\n");
	isNOP = (ID_EX.pc_branch_out) || (IF_ID.opcode_out == R && IF_ID.funct_out == SLL && IF_ID.rt_out == 0 && IF_ID.rd_out == 0 && IF_ID.shamt_out == 0);
	fprintf(snapshot, "ID: %s%s", (isNOP) ? "NOP" : (IF_ID.opcode_out == R) ? rInst[IF_ID.funct_out] : inst[IF_ID.opcode_out], (STALL) ? " to_be_stalled" : "");

	if (!STALL && !ID_EX.pc_branch_out && RS_TO_ID) fprintf(snapshot, " fwd_EX-DM_rs_$%u", IF_ID.rs_out);
	if (!STALL && !ID_EX.pc_branch_out && RT_TO_ID) fprintf(snapshot, " fwd_EX-DM_rt_$%u", IF_ID.rt_out);
	fprintf(snapshot, "\n");

	isNOP = ID_EX.opcode_out == R && ID_EX.funct_out == SLL && ID_EX.rt_out == 0 && ID_EX.rd_out == 0 && ID_EX.shamt_out == 0;
	fprintf(snapshot, "EX: %s", (isNOP) ? "NOP" : (ID_EX.opcode_out == R) ? rInst[ID_EX.funct_out] : inst[ID_EX.opcode_out]);
	
	if (RS_TO_EX) fprintf(snapshot, " fwd_EX-DM_rs_$%u", ID_EX.rs_out);
	if (RT_TO_EX) fprintf(snapshot, " fwd_EX-DM_rt_$%u", ID_EX.rt_out);
	fprintf(snapshot, "\n");

	isNOP = EX_DM.opcode_out == R && EX_DM.funct_out == SLL && EX_DM.rt_out == 0 && EX_DM.rd_out == 0 && EX_DM.shamt_out == 0;
	fprintf(snapshot, "DM: %s\n", (isNOP) ? "NOP" : (EX_DM.opcode_out == R) ? rInst[EX_DM.funct_out] : inst[EX_DM.opcode_out]);

	isNOP = DM_WB.opcode_out == R && DM_WB.funct_out == SLL && DM_WB.rt_out == 0 && DM_WB.rd_out == 0 && DM_WB.shamt_out == 0;
	fprintf(snapshot, "WB: %s\n", (isNOP) ? "NOP" : (DM_WB.opcode_out == R) ? rInst[DM_WB.funct_out] : inst[DM_WB.opcode_out]);

	fprintf(snapshot, "\n\n");
}
//Reverse transfer of inst to string
void instToString()
{
	rInst[ADD] = "ADD";
	rInst[ADDU] = "ADDU";
	rInst[SUB] = "SUB";
	rInst[AND] = "AND";
	rInst[OR] = "OR";
	rInst[XOR] = "XOR";
	rInst[NOR] = "NOR";
	rInst[NAND] = "NAND";
	rInst[SLT] = "SLT";
	rInst[SLL] = "SLL";
	rInst[SRL] = "SRL";
	rInst[SRA] = "SRA";
	rInst[JR] = "JR";
	rInst[MULT] = "MULT";
	rInst[MULTU] = "MULTU";
	rInst[MFHI] = "MFHI";
	rInst[MFLO] = "MFLO";
		 
	inst[ADDI] = "ADDI";
	inst[ADDIU] = "ADDIU";
	inst[LW] = "LW";
	inst[LH] = "LH";
	inst[LHU] = "LHU";
	inst[LB] = "LB";
	inst[LBU] = "LBU";
	inst[SW] = "SW";
	inst[SH] = "SH";
	inst[SB] = "SB";
	inst[LUI] = "LUI";
	inst[ANDI] = "ANDI";
	inst[ORI] = "ORI";
	inst[NORI] = "NORI";
	inst[SLTI] = "SLTI";
	inst[BEQ] = "BEQ";
	inst[BNE] = "BNE";
	inst[BGTZ] = "BGTZ";

	inst[J] = "J";
	inst[JAL] = "JAL";

	inst[HALT] = "HALT";
}
