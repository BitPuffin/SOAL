#include "ints.h"
#include <stddef.h>

#include <stdio.h>

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
};

struct operand {
	u8 mode;
	u8 reg;
};

struct instruction {
	u8 opcode;
	u8 operand_count;
	struct operand operands[4];
};

struct unsafevm {
	u8 stack[1024*1024];
	u64 registers[REG_COUNT];
	u8 *datasegment;
	u8 *codesegment;
	struct instruction *iptr;
};

/* data given during dispatch to instruction */
struct instruction_data {
	void *a;
	void *b;
	void *c;
	void *d;
};

struct op_add_data {
	int *src;
	int *dest;
};
void op_add_int(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_add_data *opd = (struct op_add_data *)id;
	*opd->dest = *opd->src + *opd->dest;
}

typedef void (*instruction_impl)(struct unsafevm *, struct instruction_data *);
enum opcode {
	OP_CALL,
	OP_LEAVE,
	OP_RET,
	OP_ADD_INT,
	OP_LOAD_INT,
};

instruction_impl op_impls[] = {
	NULL,
	NULL,
	NULL,
	op_add_int,
	NULL,
};

void advance_instruction(struct unsafevm *vm)
{
	struct instruction *in = vm->iptr;

	/* Move iptr forward before actual dispatch. */
	/*                                           */
	/* This is so that when when we get to a     */
	/* call instruction we can push the address  */
	/* to the next instruction on the stack.     */
	vm->iptr++;

	/* iterate over instruction_data as if it was an array */
	struct instruction_data data = {};
	void **oprdata = (void *)&data;
	struct operand *opr = in->operands;
	struct operand *end = opr + in->operand_count;
	while (opr < end) {
		/* decide if we give a pointer to the register itself     */
		/* or give a pointer stored in a register (to the operand)*/
		switch(opr->mode) {
		case MODE_REG:
			*oprdata = &vm->registers[opr->reg];
			break;
		case MODE_MEM:
			*oprdata = (void *)vm->registers[opr->reg];
			break;
		}
		opr++;
		oprdata++;
	}

	/* dispatch to instruction handler */
	op_impls[in->opcode](vm, &data);
}

void test_add() {
	struct unsafevm vm = {};
	struct instruction add = {
		OP_ADD_INT,
		2,
		{ { MODE_REG, REG_0 }, { MODE_REG, REG_2 } }
	};
	vm.registers[REG_0] = 15;
	vm.registers[REG_2] = 17;
	
	vm.iptr = &add;
	printf("before: {r0: %d} {r2: %d}\n",
	       (int) vm.registers[REG_0],
	       (int) vm.registers[REG_2]);

	advance_instruction(&vm);

	printf("after:  {r0: %d} {r2: %d}\n",
	       (int) vm.registers[REG_0],
	       (int) vm.registers[REG_2]);
}
