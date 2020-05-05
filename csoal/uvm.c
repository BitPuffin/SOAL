struct unsafevm {
	u8 *stack;
	u64 registers[REG_COUNT];
	u8 *datasegment;
	u8 *codesegment;
	struct instruction *iptr;
	DCCallVM *dyncall;
};

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

struct op_2operand_data {
	u64 *a;
	u64 *b;
};

/* @TODO: direct pointer in bytecode will probably be weird */
/* but we'll see. */
struct call_void_data {
	DCpointer *fptr;
};

/* @TODO: direct pointer in bytecode will probably be weird */
/* but we'll see. */
struct call_int_data {
	DCpointer *fptr;
	i64 *result;
};

struct c_int_arg_data {
	i64 *arg;
};

struct op_push_data {
	u64 *val;
};

struct op_add_data {
	int *src;
	int *dest;
};

typedef void (*instruction_impl)(struct unsafevm *, struct instruction_data *);


struct unsafevm	*mkuvm();
struct unsafevm	*mkuvm_stacksz(size_t sz);
static void    	 desuvm(struct unsafevm vm);

static void	 op_call(struct unsafevm *vm, struct instruction_data *id);
static void	 op_ret(struct unsafevm *vm, struct instruction_data *id);
static void	 op_c_reset(struct unsafevm *vm, struct instruction_data *id);
static void	 op_call_c_void(struct unsafevm *vm, struct instruction_data *id);
static void	 op_call_c_int(struct unsafevm *vm, struct instruction_data *id);
static void	 op_c_int_arg(struct unsafevm *vm, struct instruction_data *id);
static void	 op_add_int(struct unsafevm *vm, struct instruction_data *id);
static void	 op_push(struct unsafevm *vm, struct instruction_data *id);
static void	 op_pop(struct unsafevm *vm, struct instruction_data *id);
static void	 op_mov(struct unsafevm *vm, struct instruction_data *id);

static void	 uvm_push(struct unsafevm *vm, u64 val);
static u64	 uvm_pop(struct unsafevm *vm);

static void	 advance_instruction(struct unsafevm *vm);
static void	 run_program(struct genstate *s, size_t start_offset);


int oprcounts[] = {
	1, /* call        */
	0, /* leave       */
	0, /* ret         */
	0, /* c_reset     */
	1, /* call_c_void */
	2, /* call_c_int  */
	1, /* c_int_arg   */
	2, /* add         */
	0, /* load        */
	1, /* push        */
	1, /* pop         */
	2, /* mov         */
};

instruction_impl op_impls[] = {
	op_call,
	NULL,
	op_ret,
	op_c_reset,
	op_call_c_void,
	op_call_c_int,
	op_c_int_arg,
	op_add_int,
	NULL,
	op_push,
	op_pop,
	op_mov,
};


struct unsafevm *mkuvm()
{
	return mkuvm_stacksz(1024*1024);
}

struct unsafevm *mkuvm_stacksz(size_t sz)
{
	struct unsafevm *vm = malloc(sizeof(struct unsafevm));
	assert(vm != NULL);

	vm->stack = malloc(sz);
	assert(vm->stack != NULL);

	vm->registers[REG_SBP] = (u64)(vm->stack + sz);
	vm->registers[REG_SP]  = vm->registers[REG_SBP];

	vm->dyncall = dcNewCallVM(1024);
	assert(vm->dyncall != NULL);

	dcMode(vm->dyncall, DC_CALL_C_DEFAULT);
	dcReset(vm->dyncall);
	return vm;
}

void desuvm(struct unsafevm vm)
{
	free(vm.stack);
	dcFree(vm.dyncall);
}


void op_call(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_1operand_data *d = (struct op_1operand_data *) id;
	i64 addroffs = (i64)*d->val;
	uvm_push(vm, (u64)vm->iptr);
	vm->iptr = ((void *)(vm->iptr - 1)) + addroffs;
}

static void op_ret(struct unsafevm *vm, struct instruction_data *id)
{
	u64 i = uvm_pop(vm);
	struct instruction *newiptr = (struct instruction *)i;
	vm->iptr = newiptr;
}

static void op_c_reset(struct unsafevm *vm, struct instruction_data *id)
{
	dcReset(vm->dyncall);
}

static void op_call_c_void(struct unsafevm *vm, struct instruction_data *id)
{
	struct call_void_data *d = (struct call_void_data *)id;
	dcCallVoid(vm->dyncall, *d->fptr);
}

static void op_call_c_int(struct unsafevm *vm, struct instruction_data *id)
{
	struct call_int_data *d = (struct call_int_data *)id;
	*d->result = dcCallInt(vm->dyncall, *d->fptr);
}

static void op_c_int_arg(struct unsafevm *vm, struct instruction_data *id)
{
	struct c_int_arg_data *d = (struct c_int_arg_data *)id;
	dcArgLongLong(vm->dyncall, *d->arg);
}

static void op_add_int(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_add_data *opd = (struct op_add_data *)id;
	*opd->dest = *opd->src + *opd->dest;
}

static void op_push(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_push_data *d = (struct op_push_data *)id;
	uvm_push(vm, *d->val);
}


static void op_pop(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_1operand_data *d = (struct op_1operand_data *)id;
	*d->val = uvm_pop(vm);
}

static void op_mov(struct unsafevm *vm, struct instruction_data *id)
{
	struct op_2operand_data *d = (struct op_2operand_data *)id;
	*d->b = *d->a;
}


static void uvm_push(struct unsafevm *vm, u64 val)
{
	u64 *stack = (u64 *)vm->registers[REG_SP];
	stack--;
	vm->registers[REG_SP] = (u64)stack;
	*stack = val;
}

static u64 uvm_pop(struct unsafevm *vm)
{
	u64 *stack = (u64 *)vm->registers[REG_SP];
	u64 v = *stack;
	stack++;
	vm->registers[REG_SP] = (u64)stack;
	return v;
}


static void advance_instruction(struct unsafevm *vm)
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
			*oprdata = (void *)(vm->registers[opr->reg] + opr->offset);
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

static void run_program(struct genstate *s, size_t start_offset)
{
	struct unsafevm *vm = mkuvm();

	vm->iptr = (struct instruction *)(s->outbuf + start_offset);
	for (;;) {
		advance_instruction(vm);
	}
}
