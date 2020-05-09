
enum opcode {
	OPC_CALL,
	OPC_ENTER,
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
	OPC_MOV,

	OPC_COUNT
};

int oprcounts[] = {
	1, /* call        */
	1, /* enter       */
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

struct unsafevm;
struct instruction_data;
static void	 op_call(struct unsafevm *vm, struct instruction_data *id);
static void	 op_enter(struct unsafevm *vm, struct instruction_data *id);
static void	 op_leave(struct unsafevm *vm, struct instruction_data *id);
static void	 op_ret(struct unsafevm *vm, struct instruction_data *id);
static void	 op_c_reset(struct unsafevm *vm, struct instruction_data *id);
static void	 op_call_c_void(struct unsafevm *vm, struct instruction_data *id);
static void	 op_call_c_int(struct unsafevm *vm, struct instruction_data *id);
static void	 op_c_int_arg(struct unsafevm *vm, struct instruction_data *id);
static void	 op_add_int(struct unsafevm *vm, struct instruction_data *id);
static void	 op_push(struct unsafevm *vm, struct instruction_data *id);
static void	 op_pop(struct unsafevm *vm, struct instruction_data *id);
static void	 op_mov(struct unsafevm *vm, struct instruction_data *id);

typedef void (*instruction_impl)(struct unsafevm *, struct instruction_data *);
instruction_impl op_impls[] = {
	op_call,
	op_enter,
	op_leave,
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

const char *opc_strs[OPC_COUNT] = {
	"call",
	"enter",
	"leave",
	"ret",
	"c-reset",
	"call-c-void",
	"call-c-int",
	"c-int-arg",
	"add-int",
	"load",
	"push",
	"pop",
	"mov"
};
