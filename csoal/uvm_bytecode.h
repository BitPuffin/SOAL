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
	i32 offset;
	u64 direct_value;
};

struct instruction {
	u8 opcode;
	struct operand operands[4];
};

struct local_var_offset {
	decl_id id;
	u64 offset;
};

struct genstate {
	struct	{
		decl_id key;
		size_t value;
	}	*offset_tbl;
	u8	*outbuf;
	struct local_var_offset *local_var_offsets;
	size_t stackframe_size;
};
