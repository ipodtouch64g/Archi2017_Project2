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