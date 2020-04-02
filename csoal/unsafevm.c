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
	REG_COUNT,
};

struct instruction {
	u8 opcode;
	enum regcode reg_a;
	enum regcode reg_b;
};

struct unsafevm {
	u8 stack[1024*1024];
	u64 registers[REG_COUNT];
	u8 *datasegment;
	u8 *codesegment;
	struct instruction *iptr;
};



void op_add_int(struct unsafevm *vm, struct instruction in)
{
	printf("running add instruction\n");
	int a = vm->registers[in.reg_a];
	int b = vm->registers[in.reg_b];
	vm->registers[in.reg_b] = a + b;
}

typedef void (*instruction_impl)(struct unsafevm *, struct instruction);
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

void advance_program(struct unsafevm *vm)
{
	struct instruction in = *vm->iptr;
	vm->iptr++;
	op_impls[in.opcode](vm, in);
}


void test_add() {
	struct unsafevm vm = {};
	struct instruction add = {
		OP_ADD_INT,
		REG_0,
		REG_2
	};
	vm.registers[REG_0] = 15;
	vm.registers[REG_2] = 17;
	
	vm.iptr = &add;
	printf("before: {r0: %d} {r2: %d}\n",
	       (int) vm.registers[REG_0],
	       (int) vm.registers[REG_2]);

	advance_program(&vm);

	printf("after:  {r0: %d} {r2: %d}\n",
	       (int) vm.registers[REG_0],
	       (int) vm.registers[REG_2]);
}





