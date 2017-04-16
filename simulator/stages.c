#include "stages.h"

void update() {
	ID_EX.opcode_in = IF_ID.opcode_out;
	ID_EX.rs_in = IF_ID.rs_out;
	ID_EX.rt_in = IF_ID.rt_out;
	ID_EX.rd_in = IF_ID.rd_out;
	ID_EX.funct_in = IF_ID.funct_out;
	ID_EX.shamt_in = IF_ID.shamt_out;
	ID_EX.pc_plus_four_in = IF_ID.pc_plus_four_out;
	
	EX_DM.opcode_in = ID_EX.opcode_out;
	EX_DM.rs_in = ID_EX.rs_out;
	EX_DM.rd_in = ID_EX.rd_out;
	EX_DM.rt_in = ID_EX.rt_out;
	EX_DM.funct_in = ID_EX.funct_out;
	EX_DM.shamt_in = ID_EX.shamt_out;
	EX_DM.num_rs_in = ID_EX.num_rs_out;
	EX_DM.num_rt_in = ID_EX.num_rt_out;
	EX_DM.reg_to_write_in = ID_EX.reg_to_write_out;

	DM_WB.opcode_in = EX_DM.opcode_out;
	DM_WB.rs_in = EX_DM.rs_out;
	DM_WB.rt_in = EX_DM.rt_out;
	DM_WB.rd_in = EX_DM.rd_out;
	DM_WB.shamt_in = EX_DM.shamt_out;
	DM_WB.funct_in = EX_DM.funct_out;
	DM_WB.alu_result_in = EX_DM.alu_result_out;
	DM_WB.reg_to_write_in = EX_DM.reg_to_write_out;
}
inline void detectWriteToZero() {

	if (DM_WB.reg_to_write_out == 0) {
		if (DM_WB.opcode_out == R && DM_WB.funct_out == SLL) {
			if (!(DM_WB.rt_out == 0 && DM_WB.shamt_out == 0)) {
				writeRegZero = 1;
				reg[DM_WB.reg_to_write_out] = 0;
			}
		}
		else {
			writeRegZero = 1;
			reg[DM_WB.reg_to_write_out] = 0;
		}
	}

}

inline void detectNumberOverflow(int a, int b, int c) {
	if ((a>0 && b>0 && c<0) || (a<0 && b<0 && c>0)) {
		numOverflow = 1;
	}
}

inline int detectMemoryOverflow(int n) {
	if (EX_DM.alu_result_out + n >= 1024 || EX_DM.alu_result_out >= 1024 || EX_DM.alu_result_out < 0) {
		memOverflow = 1;
		ERROR_HALT = 1;
		return 1;
	}
	else return 0;
}

inline int detectDataMisaaligned(int n) {
	if (EX_DM.alu_result_out % (n + 1) > 0) {
		dataMisaligned = 1;
		ERROR_HALT = 1;
		return 1;
	}
	else return 0;
}

void IF() {
	if (STALL) {
		//printf("IF STALL .....\n");
		return;
	}
	if (ID_EX.pc_branch_out == 1) {
		PC = ID_EX.pc_out;
		//printf("ID_EX.pc_branch_out=1, PC= 0x%08X\n",PC);
	}
	else {
		PC = IF_ID.pc_plus_four_out;
		//printf("ID_EX.pc_branch_out=0, IF_ID.pc_plus_four_out=0x%08X, PC= 0x%08X\n",IF_ID.pc_plus_four_out,PC);
	}

	//printf("PC: 0x%08X\n", PC);

	for (int i = 0; i < 4; i++)
		IF_ID.inst_in = (IF_ID.inst_in << 8) | (unsigned char)iMemory[PC + i];

	unsigned t1, t2;
	IF_ID.opcode_in = iMemory[PC];
	IF_ID.opcode_in = IF_ID.opcode_in >> 2 << 26 >> 26;

	IF_ID.funct_in = iMemory[PC + 3];
	IF_ID.funct_in = IF_ID.funct_in << 26 >> 26;

	t1 = iMemory[PC], t2 = iMemory[PC + 1];
	t1 = t1 << 30 >> 27;
	t2 = t2 << 24 >> 29;
	IF_ID.rs_in = t1 + t2;

	IF_ID.rt_in = iMemory[PC + 1];
	IF_ID.rt_in = IF_ID.rt_in << 27 >> 27;

	IF_ID.rd_in = iMemory[PC + 2];
	IF_ID.rd_in = IF_ID.rd_in << 24 >> 27;

	t1 = iMemory[PC + 2], t2 = iMemory[PC + 3];
	t1 = t1 << 29 >> 27;
	t2 = t2 >> 6 << 30 >> 30;
	IF_ID.shamt_in = t1 + t2;

	PC = PC + 4;
	IF_ID.pc_plus_four_in = PC;
}

void ID() {
	checkStall();

	if (STALL) return;
	if (ID_EX.pc_branch_out) { // need to branch
		ID_EX.pc_plus_four_in = 0;
		ID_EX.opcode_in = 0;
		ID_EX.funct_in = 0;
		ID_EX.shamt_in = 0;
		ID_EX.rs_in = 0;
		ID_EX.rt_in = 0;
		ID_EX.rd_in = 0;
		ID_EX.pc_branch_in = 0;
		ID_EX.pc_in = 0;
		ID_EX.reg_to_write_in = 0;
		ID_EX.num_rs_in = 0;
		ID_EX.num_rt_in = 0;
		ID_EX.extended_imme_in = 0;

		return;
	}

	checkForwardToID();
	ID_EX.num_rs_in = (RS_TO_ID) ? EX_DM.alu_result_out : reg[ID_EX.rs_in];
	ID_EX.num_rt_in = (RT_TO_ID) ? EX_DM.alu_result_out : reg[ID_EX.rt_in];

	ID_EX.extended_imme_in = (short)IF_ID.inst_out << 16 >> 16;

	ID_EX.pc_branch_in = 0;
	/* Branch */
	if (ID_EX.opcode_in == BGTZ) {
		if ((int)ID_EX.num_rs_in > 0) {
			ID_EX.pc_branch_in = 1;
			ID_EX.pc_in = IF_ID.pc_plus_four_out + 4 * ID_EX.extended_imme_in;
		}
	}
	else if (ID_EX.opcode_in == BNE) {
		if (ID_EX.num_rs_in != ID_EX.num_rt_in) {
			ID_EX.pc_branch_in = 1;
			ID_EX.pc_in = IF_ID.pc_plus_four_out + 4 * ID_EX.extended_imme_in;
		}
	}
	else if (ID_EX.opcode_in == BEQ) {
		if (ID_EX.num_rs_in == ID_EX.num_rt_in) {
			ID_EX.pc_branch_in = 1;
			ID_EX.pc_in = IF_ID.pc_plus_four_out + 4 * ID_EX.extended_imme_in;
		}
	}
	else if (ID_EX.opcode_in == J || ID_EX.opcode_in == JAL) {
		unsigned address = IF_ID.inst_out << 6 >> 6;
		ID_EX.pc_branch_in = 1;
		ID_EX.pc_in = (IF_ID.pc_plus_four_out >> 28 << 28) | (address << 2);
	}
	else if (ID_EX.opcode_in == R && ID_EX.funct_in == JR) {
		ID_EX.pc_branch_in = 1;
		ID_EX.pc_in = ID_EX.num_rs_in;
	}

	if (ID_EX.opcode_in != R && ID_EX.opcode_in != JAL) ID_EX.reg_to_write_in = ID_EX.rt_in;
	else if (ID_EX.opcode_in == R) ID_EX.reg_to_write_in = ID_EX.rd_in;
	else if (ID_EX.opcode_in == JAL) ID_EX.reg_to_write_in = 31;
}

void EX() {
	/* store */
	if (cycle == 59) {
		printf(" in EX before c59 ID_EX.extended_imme_out = %d ID_EX.num_rt_out %d\n", ID_EX.extended_imme_out, ID_EX.num_rt_out);
	}
	int tmp = ID_EX.num_rt_out;

	checkForwardToEX();

	unsigned left, right;
	if (EX_DM.opcode_in == SW || EX_DM.opcode_in == SH || EX_DM.opcode_in == SB) {
		if (RS_TO_EX) /*ID_EX.num_rs_out*/left = EX_DM.alu_result_out;
		else left = ID_EX.num_rs_out;
		/*ID_EX.num_rt_out*/ right = ID_EX.extended_imme_out;

		if (cycle == 59) {
			printf(" in EX after c59 ID_EX.extended_imme_out = %d ID_EX.num_rt_out %d\n", ID_EX.extended_imme_out, ID_EX.num_rt_out);
		}
		if (RT_TO_EX) EX_DM.write_data_in = EX_DM.alu_result_out;
		else EX_DM.write_data_in = tmp;//EX_DM.write_data_in = ID_EX.num_rt_out;

		if (cycle == 59) {
			printf("EX_DM.write_data_in = %d ID_EX.num_rt_out %d\n", EX_DM.write_data_in, ID_EX.num_rt_out);
		}
	}
	else {
		if (RS_TO_EX) /*ID_EX.num_rs_out*/ left = EX_DM.alu_result_out;
		else left = ID_EX.num_rs_out;
		if (ID_EX.opcode_out == R) {
			if (RT_TO_EX) /*ID_EX.num_rt_out*/ right = EX_DM.alu_result_out;
			else right = ID_EX.num_rt_out;
		}
		else /*ID_EX.num_rt_out*/ right = ID_EX.extended_imme_out;
	}

	unsigned left_sign;
	unsigned right_sign;
	unsigned result_sign;
	signed int Ileft = (int)left;//(int)ID_EX.num_rs_out;
	signed int Iright = (int)right;//(int)ID_EX.num_rt_out;
	signed int Iresult = Ileft + Iright;

	switch (EX_DM.opcode_in) {
	case R:
		switch (EX_DM.funct_in) {
		case ADD:
			left_sign = left >> 31, right_sign = right >> 31;
			EX_DM.alu_result_in = left + right;
			result_sign = EX_DM.alu_result_in >> 31;
			if (left_sign == right_sign && left_sign != result_sign) {
				numOverflow = 1;
			}
			break;
		case ADDU:
			EX_DM.alu_result_in = left + right;
			break;
		case SUB:
			left_sign = (left >> 31);
			right_sign = ((-right) >> 31);
			EX_DM.alu_result_in = left - right;
			result_sign = EX_DM.alu_result_in >> 31;
			if (left_sign == right_sign && left_sign != result_sign) {
				numOverflow = 1;
			}
			break;
		case AND:
			EX_DM.alu_result_in = left & right;
			break;
		case OR:
			EX_DM.alu_result_in = left | right;
			break;
		case XOR:
			EX_DM.alu_result_in = left ^ right;
			break;
		case NOR:
			EX_DM.alu_result_in = ~(left | right);
			break;
		case NAND:
			EX_DM.alu_result_in = ~(left & right);
			break;
		case SLT:
			EX_DM.alu_result_in = ((int)left < (int)right) ? 1 : 0;
			break;
		case SLL:
			EX_DM.alu_result_in = right << EX_DM.shamt_in;
			break;
		case SRL:
			EX_DM.alu_result_in = right >> EX_DM.shamt_in;
			break;
		case SRA:
			EX_DM.alu_result_in = (int)right >> EX_DM.shamt_in;
			break;
		default:
			break;
		}
		break;
	case ADDI:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case ADDIU:
		EX_DM.alu_result_in = left + right;
		break;
	case LW:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case LH:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case LHU:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case LB:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case LBU:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case SW:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case SH:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case SB:
		EX_DM.alu_result_in = left + right;
		detectNumberOverflow(Ileft, Iright, Iresult);
		break;
	case LUI:
		EX_DM.alu_result_in = right << 16;
		break;
	case ANDI:
		EX_DM.alu_result_in = left & (unsigned short)right;
		break;
	case ORI:
		EX_DM.alu_result_in = left | (unsigned short)right;
		break;
	case NORI:
		EX_DM.alu_result_in = ~(left | (unsigned short)right);
		break;
	case SLTI:
		EX_DM.alu_result_in = ((int)left < (int)right) ? 1 : 0;
		break;
	case JAL:
		EX_DM.alu_result_in = ID_EX.pc_plus_four_out;
	default:
		break;
	}
}

void DM() {
	int isMemoryOverflow = 0, isDataMisaaligned = 0;
	unsigned t1, t2, t3, t4;
	int inT1, inT2, inT3, inT4;

	if (EX_DM.opcode_out == R) return; /* only load, store */

	switch (EX_DM.opcode_out) {
	case LW:
		isMemoryOverflow = detectMemoryOverflow(3);
		isDataMisaaligned = detectDataMisaaligned(3);
		if (!isMemoryOverflow && !isDataMisaaligned) {
			t1 = dMemory[EX_DM.alu_result_out] << 24;
			t2 = dMemory[EX_DM.alu_result_out + 1] << 16;
			t3 = dMemory[EX_DM.alu_result_out + 2] << 8;
			t4 = dMemory[EX_DM.alu_result_out + 3];
			DM_WB.read_data_in = t1 + t2 + t3 + t4;
			if (cycle == 70) {
				printf("GOOD!\n");
			}
		}
		if (cycle == 70) {
			printf("It's LW\n");
			printf("DMemory[980] = %d\n", dMemory[980]);
			printf("DM_WB.read_data_in = %d\n", DM_WB.read_data_in);
			printf("EX_DM.alu_result_out = %d\n", EX_DM.alu_result_out);
		}
		break;
	case LH:
		isMemoryOverflow = detectMemoryOverflow(1);
		isDataMisaaligned = detectDataMisaaligned(1);
		if (!isMemoryOverflow && !isDataMisaaligned) {
			t1 = dMemory[EX_DM.alu_result_out];
			t1 = t1 << 24 >> 16;
			t2 = dMemory[EX_DM.alu_result_out + 1];
			t2 = t2 << 24 >> 24;
			DM_WB.read_data_in = (short)(t1 + t2);
		}
		break;
	case LHU:
		isMemoryOverflow = detectMemoryOverflow(1);
		isDataMisaaligned = detectDataMisaaligned(1);
		if (isMemoryOverflow || isDataMisaaligned) {
			t1 = dMemory[EX_DM.alu_result_out] << 8;//<< 24 >> 16;
			t2 = dMemory[EX_DM.alu_result_out + 1]; //<< 24 >> 24;
			DM_WB.read_data_in = t1 + t2;
		}
		break;
	case LB:
		isMemoryOverflow = detectMemoryOverflow(0);
		isDataMisaaligned = detectDataMisaaligned(0);
		if (!isMemoryOverflow && !isDataMisaaligned) {
			inT1 = dMemory[EX_DM.alu_result_out];
			inT1 = inT1 << 24 >> 24;
			DM_WB.read_data_in = inT1;
		}
		break;
	case LBU:
		isMemoryOverflow = detectMemoryOverflow(0);
		isDataMisaaligned = detectDataMisaaligned(0);
		if (!isMemoryOverflow && !isDataMisaaligned) {
			DM_WB.read_data_in = dMemory[EX_DM.alu_result_out];
		}
		break;
	case SW:
		isMemoryOverflow = detectMemoryOverflow(3);
		isDataMisaaligned = detectDataMisaaligned(3);
		if (!isMemoryOverflow && !isDataMisaaligned) {
			dMemory[EX_DM.alu_result_out] = EX_DM.write_data_out >> 24;
			dMemory[EX_DM.alu_result_out + 1] = EX_DM.write_data_out << 8 >> 24;
			dMemory[EX_DM.alu_result_out + 2] = EX_DM.write_data_out << 16 >> 24;
			dMemory[EX_DM.alu_result_out + 3] = EX_DM.write_data_out << 24 >> 24;
			if (cycle == 60) {
				printf("It's SW\n");
				printf("EX_DM.write_data_out = %d\n", EX_DM.write_data_out);
			}
		}
		break;
	case SH:
		isMemoryOverflow = detectMemoryOverflow(1);
		isDataMisaaligned = detectDataMisaaligned(1);
		if (!isMemoryOverflow && !isDataMisaaligned) {
			dMemory[EX_DM.alu_result_out] = (EX_DM.write_data_out >> 8) & 0xff;
			dMemory[EX_DM.alu_result_out + 1] = EX_DM.write_data_out;
		}
		break;
	case SB:
		isMemoryOverflow = detectMemoryOverflow(0);
		isDataMisaaligned = detectDataMisaaligned(0);
		if (!isMemoryOverflow && !isDataMisaaligned) {
			dMemory[EX_DM.alu_result_out] = EX_DM.write_data_out;
		}
		break;
	default:
		break;
	}
}

void WB() {
	if (cycle == 71) {
		printf("It's WB\n");
		printf("DM_WB.reg_to_write_out = %d\n", DM_WB.reg_to_write_out);
		printf("DM_WB.read_data_out = %d\n", DM_WB.read_data_out);
	}

	int writeBackReg = (DM_WB.opcode_out == R && DM_WB.funct_out != JR) || (DM_WB.opcode_out != R && DM_WB.opcode_out != HALT && DM_WB.opcode_out != J && DM_WB.opcode_out != BGTZ && DM_WB.opcode_out != BNE && DM_WB.opcode_out != BEQ && DM_WB.opcode_out != SB && DM_WB.opcode_out != SW && DM_WB.opcode_out != SH);
	if (writeBackReg) {
		/*load*/
		if (DM_WB.opcode_out == LW || DM_WB.opcode_out == LH || DM_WB.opcode_out == LHU || DM_WB.opcode_out == LB || DM_WB.opcode_out == LBU) {
			reg[DM_WB.reg_to_write_out] = DM_WB.read_data_out;
		}
		else reg[DM_WB.reg_to_write_out] = DM_WB.alu_result_out;

		detectWriteToZero();
	}

	if (DM_WB.opcode_out == HALT) WB_HALT = 1;
	else WB_HALT = 0;
}

void move() {
	//DM2WB
	DM_WB.opcode_out = DM_WB.opcode_in;
	DM_WB.rs_out = DM_WB.rs_in;
	DM_WB.rt_out = DM_WB.rt_in;
	DM_WB.rd_out = DM_WB.rd_in;
	DM_WB.funct_out = DM_WB.funct_in;
	DM_WB.shamt_out = DM_WB.shamt_in;

	DM_WB.alu_result_out = DM_WB.alu_result_in;
	DM_WB.reg_to_write_out = DM_WB.reg_to_write_in;
	DM_WB.read_data_out = DM_WB.read_data_in;

	//EX2DM
	EX_DM.opcode_out = EX_DM.opcode_in;
	EX_DM.rs_out = EX_DM.rs_in;
	EX_DM.rt_out = EX_DM.rt_in;
	EX_DM.rd_out = EX_DM.rd_in;
	EX_DM.shamt_out = EX_DM.shamt_in;
	EX_DM.funct_out = EX_DM.funct_in;

	EX_DM.num_rs_out = EX_DM.num_rs_in;
	EX_DM.num_rt_out = EX_DM.num_rt_in;
	EX_DM.reg_to_write_out = EX_DM.reg_to_write_in;
	EX_DM.alu_result_out = EX_DM.alu_result_in;

	EX_DM.write_data_out = EX_DM.write_data_in;

	//ID2EX
	if (STALL) {
		ID_EX.pc_plus_four_out = 0;
		ID_EX.opcode_out = 0;
		ID_EX.rs_out = 0;
		ID_EX.rt_out = 0;
		ID_EX.rd_out = 0;
		ID_EX.funct_out = 0;
		ID_EX.shamt_out = 0;

		ID_EX.num_rs_out = 0;
		ID_EX.num_rt_out = 0;
		ID_EX.extended_imme_out = 0;

		ID_EX.pc_branch_out = 0;
		ID_EX.pc_out = 0;
		ID_EX.reg_to_write_out = 0;

	}
	else {
		ID_EX.pc_plus_four_out = ID_EX.pc_plus_four_in;
		ID_EX.opcode_out = ID_EX.opcode_in;
		ID_EX.rs_out = ID_EX.rs_in;
		ID_EX.rt_out = ID_EX.rt_in;
		ID_EX.rd_out = ID_EX.rd_in;
		ID_EX.funct_out = ID_EX.funct_in;
		ID_EX.shamt_out = ID_EX.shamt_in;

		ID_EX.num_rs_out = ID_EX.num_rs_in;
		ID_EX.num_rt_out = ID_EX.num_rt_in;
		ID_EX.extended_imme_out = ID_EX.extended_imme_in;

		ID_EX.pc_branch_out = ID_EX.pc_branch_in;
		ID_EX.pc_out = ID_EX.pc_in;
		ID_EX.reg_to_write_out = ID_EX.reg_to_write_in;

		if (cycle == 58) {
			printf("ID_EX.num_rt_out = %d\n", ID_EX.num_rt_out);
		}
	}

	//IF2ID
	if (STALL) return;
	IF_ID.pc_plus_four_out = IF_ID.pc_plus_four_in;
	IF_ID.inst_out = IF_ID.inst_in;

	IF_ID.opcode_out = IF_ID.opcode_in;
	IF_ID.rs_out = IF_ID.rs_in;
	IF_ID.rt_out = IF_ID.rt_in;
	IF_ID.rd_out = IF_ID.rd_in;
	IF_ID.funct_out = IF_ID.funct_in;
	IF_ID.shamt_out = IF_ID.shamt_in;
}
void checkStall() {
	STALL = 0;
	if (ID_EX.pc_branch_out == 1) return;

	int IDLoad = (ID_EX.opcode_out == LW || ID_EX.opcode_out == LH || ID_EX.opcode_out == LHU || ID_EX.opcode_out == LB || ID_EX.opcode_out == LBU);
	int EXLoad = (EX_DM.opcode_out == LW || EX_DM.opcode_out == LH || EX_DM.opcode_out == LHU || EX_DM.opcode_out == LB || EX_DM.opcode_out == LBU);

	int EXwriteReg = (EX_DM.opcode_out == R && EX_DM.funct_out != JR) || (EX_DM.opcode_out != R && EX_DM.opcode_out != HALT && EX_DM.opcode_out != J && EX_DM.opcode_out != BGTZ && EX_DM.opcode_out != BNE && EX_DM.opcode_out != BEQ && EX_DM.opcode_out != SB && EX_DM.opcode_out != SW && EX_DM.opcode_out != SH);
	int IDwriteReg = (ID_EX.opcode_out == R && ID_EX.funct_out != JR) || (ID_EX.opcode_out != R && ID_EX.opcode_out != HALT && ID_EX.opcode_out != J && ID_EX.opcode_out != BGTZ && ID_EX.opcode_out != BNE && ID_EX.opcode_out != BEQ && ID_EX.opcode_out != SB && ID_EX.opcode_out != SW && ID_EX.opcode_out != SH);

	// EX_DM to ID (not branches) stall
	if (IF_ID.opcode_out == R && IF_ID.funct_out != SLL && IF_ID.funct_out != SRL && IF_ID.funct_out != SRA && IF_ID.funct_out != JR) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out))) STALL = 1;
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rt_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out))) STALL = 1;
	}
	else if (IF_ID.opcode_out == R && (IF_ID.funct_out == SLL || IF_ID.funct_out == SRL || IF_ID.funct_out == SRA)) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rt_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out))) STALL = 1;
	}
	else if (IF_ID.opcode_out != R && IF_ID.opcode_out != LUI && IF_ID.opcode_out != BEQ && IF_ID.opcode_out != BNE && IF_ID.opcode_out != BGTZ
		&& IF_ID.opcode_out != J && IF_ID.opcode_out != JAL && IF_ID.opcode_out != HALT && IF_ID.opcode_out != SW && IF_ID.opcode_out != SH && IF_ID.opcode_out != SB) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out))) STALL = 1;
	}
	else if (IF_ID.opcode_out == SW || IF_ID.opcode_out == SH || IF_ID.opcode_out == SB) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out))) STALL = 1;
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rt_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out))) STALL = 1;
	}
	// ID_EX to ID (branches) stall
	if (IF_ID.opcode_out == BEQ || IF_ID.opcode_out == BNE) {
		if (IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
		if (IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out)) STALL = 1;
	}
	else if (IF_ID.opcode_out == BGTZ) {
		if (IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
	}
	else if (IF_ID.opcode_out == R && IF_ID.funct_out == JR) {
		if (IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
	}
	// memory read stall
	if (IF_ID.opcode_out == R && IF_ID.funct_out != SLL && IF_ID.funct_out != SRL && IF_ID.funct_out != SRA && IF_ID.funct_out != JR) {
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out))) STALL = 1;
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rt_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out))) STALL = 1;
	}
	else if (IF_ID.opcode_out == R && (IF_ID.funct_out == SLL || IF_ID.funct_out == SRL || IF_ID.funct_out == SRA)) {
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rt_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out))) STALL = 1;
	}
	else if (IF_ID.opcode_out != R && IF_ID.opcode_out != LUI && IF_ID.opcode_out != BEQ && IF_ID.opcode_out != BNE && IF_ID.opcode_out != BGTZ
		&& IF_ID.opcode_out != J && IF_ID.opcode_out != JAL && IF_ID.opcode_out != HALT && IF_ID.opcode_out != SW && IF_ID.opcode_out != SH && IF_ID.opcode_out != SB) {
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out))) STALL = 1;
	}
	else if (IF_ID.opcode_out == SW || IF_ID.opcode_out == SH || IF_ID.opcode_out == SB) {
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out))) STALL = 1;
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rt_out) && !(IDwriteReg && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out))) STALL = 1;
	}
	else if (IF_ID.opcode_out == BNE || IF_ID.opcode_out == BEQ) {
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rt_out)) STALL = 1;
	}
	else if (IF_ID.opcode_out == BGTZ || (IF_ID.opcode_out == R && IF_ID.funct_out == JR)) {
		if (EXLoad && (EX_DM.reg_to_write_out != 0) && (EX_DM.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
	}

	// memory read stall
	if (IF_ID.opcode_out == R && IF_ID.funct_out != SLL && IF_ID.funct_out != SRL && IF_ID.funct_out != SRA && IF_ID.funct_out != JR) {
		if (IDLoad && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
		if (IDLoad && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out)) STALL = 1;
	}
	else if (IF_ID.opcode_out == R && (IF_ID.funct_out == SLL || IF_ID.funct_out == SRL || IF_ID.funct_out == SRA)) {
		if (IDLoad && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out)) STALL = 1;
	}
	else if (IF_ID.opcode_out != R && IF_ID.opcode_out != LUI && IF_ID.opcode_out != BEQ && IF_ID.opcode_out != BNE && IF_ID.opcode_out != BGTZ
		&& IF_ID.opcode_out != J && IF_ID.opcode_out != JAL && IF_ID.opcode_out != HALT && IF_ID.opcode_out != SW && IF_ID.opcode_out != SH && IF_ID.opcode_out != SB) {
		if (IDLoad && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
	}
	else if (IF_ID.opcode_out == BNE || IF_ID.opcode_out == BEQ || IF_ID.opcode_out == SW || IF_ID.opcode_out == SH || IF_ID.opcode_out == SB) {
		if (IDLoad && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
		if (IDLoad && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rt_out)) STALL = 1;
	}
	else if (IF_ID.opcode_out == BGTZ || (IF_ID.opcode_out == R && IF_ID.funct_out == JR)) {
		if (IDLoad && (ID_EX.reg_to_write_out != 0) && (ID_EX.reg_to_write_out == IF_ID.rs_out)) STALL = 1;
	}
}

void checkForwardToID() {
	RS_TO_ID = 0;
	RT_TO_ID = 0;

	int EXwriteReg = (EX_DM.opcode_out == R && EX_DM.funct_out != JR) || (EX_DM.opcode_out != R && EX_DM.opcode_out != HALT && EX_DM.opcode_out != J && EX_DM.opcode_out != BGTZ && EX_DM.opcode_out != BNE && EX_DM.opcode_out != BEQ && EX_DM.opcode_out != SB && EX_DM.opcode_out != SW && EX_DM.opcode_out != SH);
	if (IF_ID.opcode_out == BEQ || IF_ID.opcode_out == BNE) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0)) {
			if (EX_DM.reg_to_write_out == IF_ID.rt_out) RT_TO_ID = 1;
			if (EX_DM.reg_to_write_out == IF_ID.rs_out) RS_TO_ID = 1;
		}
	}
	else if (IF_ID.opcode_out == BGTZ) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0)) {
			if (EX_DM.reg_to_write_out == IF_ID.rs_out) RS_TO_ID = 1;
		}
	}
	else if (IF_ID.opcode_out == R && IF_ID.funct_out == JR) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0)) {
			if (EX_DM.reg_to_write_out == IF_ID.rs_out) RS_TO_ID = 1;
		}
	}
}

void checkForwardToEX() {
	RS_TO_EX = 0;
	RT_TO_EX = 0;

	int EXwriteReg = (EX_DM.opcode_out == R && EX_DM.funct_out != JR) || (EX_DM.opcode_out != R && EX_DM.opcode_out != HALT && EX_DM.opcode_out != J && EX_DM.opcode_out != BGTZ && EX_DM.opcode_out != BNE && EX_DM.opcode_out != BEQ && EX_DM.opcode_out != SB && EX_DM.opcode_out != SW && EX_DM.opcode_out != SH);
	int IDwriteReg = (ID_EX.opcode_out == R && ID_EX.funct_out != JR) || (ID_EX.opcode_out != R && ID_EX.opcode_out != HALT && ID_EX.opcode_out != J && ID_EX.opcode_out != BGTZ && ID_EX.opcode_out != BNE && ID_EX.opcode_out != BEQ && ID_EX.opcode_out != SB && ID_EX.opcode_out != SW && ID_EX.opcode_out != SH);

	if (ID_EX.opcode_out == R) {
		if (ID_EX.funct_out == SLL || ID_EX.funct_out == SRL || ID_EX.funct_out == SRA) {
			if (EXwriteReg && (EX_DM.reg_to_write_out != 0)) {
				if (EX_DM.reg_to_write_out == ID_EX.rt_out) RT_TO_EX = 1;
			}
		}
		else if (ID_EX.funct_out != JR) {
			if (EXwriteReg && (EX_DM.reg_to_write_out != 0)) {
				if (EX_DM.reg_to_write_out == ID_EX.rs_out) RS_TO_EX = 1;
				if (EX_DM.reg_to_write_out == ID_EX.rt_out) RT_TO_EX = 1;
			}
		}
	}
	else if (ID_EX.opcode_out != R && ID_EX.opcode_out != LUI && ID_EX.opcode_out != BEQ && ID_EX.opcode_out != BNE && ID_EX.opcode_out != BGTZ
		&& ID_EX.opcode_out != J && ID_EX.opcode_out != JAL && ID_EX.opcode_out != HALT && ID_EX.opcode_out != SW && ID_EX.opcode_out != SH && ID_EX.opcode_out != SB) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0)) {
			if (EX_DM.reg_to_write_out == ID_EX.rs_out) RS_TO_EX = 1;
		}
	}
	else if (ID_EX.opcode_out == SW || ID_EX.opcode_out == SH || ID_EX.opcode_out == SB) {
		if (EXwriteReg && (EX_DM.reg_to_write_out != 0)) {
			if (EX_DM.reg_to_write_out == ID_EX.rs_out) RS_TO_EX = 1;
			if (EX_DM.reg_to_write_out == ID_EX.rt_out) RT_TO_EX = 1;
		}

	}
}