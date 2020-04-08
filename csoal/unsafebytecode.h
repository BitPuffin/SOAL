/* codes for registers */
enum regcode {
	REG_0,
	REG_1,
	REG_2,
	REG_3,
	REG_4,
	REG_5,
	REG_6,
	REG_7,
	REG_8,
	REG_9,
	REG_10,
	REG_11,
	REG_12,
	REG_13,
	REG_14,
	REG_15,

	REG_SP, /* stack pointer */
	REG_SBP, /* stack base pointer */

	REG_COUNT,
};


enum operand_mode {
	MODE_REG,
	MODE_MEM,
	MODE_DIRECT,
};

struct operand {
	u8 mode;
	u8 reg;
	u16 __padding__;
	u64 direct_value;
};


struct instruction {
	u8 opcode;
	struct operand operands[4];
};

enum opcode {
	OPC_CALL,
	OPC_LEAVE,
	OPC_RET,
	OPC_C_RESET,
	OPC_CALL_C_VOID,
	OPC_CALL_C_INT,
	OPC_C_INT_ARG,
	OPC_ADD_INT,
	OPC_LOAD_INT,
	OPC_PUSH,
	OPC_POP,
};
