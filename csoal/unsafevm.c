#include "ints.h"
#include <stddef.h>
#include <stdlib.h>

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

struct unsafevm {
	u8 *stack;
	u64 registers[REG_COUNT];
	u8 *datasegment;
	u8 *codesegment;
	struct instruction *iptr;
};

struct unsafevm mkuvm_stacksz(size_t sz);
struct unsafevm mkuvm()
{
	return mkuvm_stacksz(1024*1024);
}

struct unsafevm mkuvm_stacksz(size_t sz)
{
	struct unsafevm vm = {};
	vm.stack = malloc(sz);
	vm.registers[REG_SBP] = (u64)(vm.stack + sz);
	vm.registers[REG_SP]  = vm.registers[REG_SBP];
	return vm;
}

void destroyuvm(struct unsafevm vm)
{
	free(vm.stack);
}

/* data given during dispatch to instruction */
struct instruction_data {
	void *a;
	void *b;
	void *c;
	void *d;
};

struct op_1operand_data {
	u64 *val;
};

static void push(struct unsafevm *vm, u64 v);
void op_call(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_1operand_data *d = (struct op_1operand_data *) id;
	i64 addroffs = (i64)*d->val;
	push(vm, (u64)vm->iptr);
	vm->iptr += addroffs;
}

struct op_add_data {
	int *src;
	int *dest;
};
void op_add_int(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_add_data *opd = (struct op_add_data *)id;
	*opd->dest = *opd->src + *opd->dest;
}

struct op_push_data {
	u64 *val;
};
static void push(struct unsafevm *vm, u64 val)
{
	u64 *stack = (u64 *)vm->registers[REG_SP];
	stack--;
	vm->registers[REG_SP] = (u64)stack;
	*stack = val;
}
void op_push(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_push_data *d = (struct op_push_data *)id;
	push(vm, *d->val);
}

static u64 pop(struct unsafevm *vm)
{
	u64 *stack = (u64 *)vm->registers[REG_SP];
	u64 v = *stack;
	stack++;
	vm->registers[REG_SP] = (u64)stack;
	return v;
}

void op_pop(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_1operand_data *d = (struct op_1operand_data *)id;
	*d->val = pop(vm);
}

typedef void (*instruction_impl)(struct unsafevm *, struct instruction_data *);
enum opcode {
	OPC_CALL,
	OPC_LEAVE,
	OPC_RET,
	OPC_ADD_INT,
	OPC_LOAD_INT,
	OPC_PUSH,
	OPC_POP,
};

instruction_impl op_impls[] = {
	op_call,
	NULL,
	NULL,
	op_add_int,
	NULL,
	op_push,
	op_pop,
};

u8 oprcounts[] = {
	1, /*      */
	0, /*      */
	0, /*      */
	2, /* add  */
	0, /*      */
	1, /* push */
	1, /* pop  */
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
	struct operand *end = opr + oprcounts[in->opcode];
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
		case MODE_DIRECT:
			*oprdata = &opr->direct_value;
		}
		opr++;
		oprdata++;
	}

	/* dispatch to instruction handler */
	op_impls[in->opcode](vm, &data);
}

