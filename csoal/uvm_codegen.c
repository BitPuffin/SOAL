#define NOT_FOUND SIZE_MAX

struct c_table_entry {
	char *name;
	void *fptr;
};

void	 __prntnum(i64 n);
void	 _builtin_exit();
void	 __newline();

static void	*getcfn(char *name);
static void	 emit_instruction(struct genstate *s, struct instruction *i);
static void	 emit_push(struct genstate *s, struct operand o);
static void	 emit_pop(struct genstate *s, struct operand o);
static void	 emit_ret(struct genstate *s);
static void	 emit_pop_into_reg(struct genstate *s, enum regcode r);
static void emit_c_int_arg_opr(struct genstate *s, struct operand opr);
static void	 emit_c_int_arg_from_reg(struct genstate *s, enum regcode r);
static void	 emit_c_int_arg_direct(struct genstate *s, i64 v);
static void	 emit_c_call_void_direct(struct genstate *s, u64 data);
static void	 emit_c_reset(struct genstate *s);
static void	 emit_form_call(struct genstate *s, struct scope *scope, struct formnode *fnp);
static u64 offset_of_local_var(struct genstate *s, decl_id id);
static void	 emit_block(struct genstate *s, struct blocknode *bnp);
static void	 emit_block_constants(struct genstate *s, struct blocknode *np);
static void	 emit_proc(struct genstate *s, struct procnode *pnp);
static void	 emit_raw_data(struct genstate *s, size_t sz, void *d);
static void	 emit_integer(struct genstate *s, i64 integer);
static void	 emit_def(struct genstate *s, struct scope *scope, struct defnode *dnp);
static void	 emit_toplevel(struct genstate *gst, struct toplevelnode *tlnp);
static struct genstate	 emit_bytecode(struct toplevelnode *tlnp);
static size_t	 emit_entry_point(struct genstate *gsp, size_t main_offset);


struct c_table_entry c_fn_tbl[] = {
	{ .name = "print-number", .fptr = __prntnum },
	{ .name = "exit", .fptr = _builtin_exit },
	{ .name = "newline", .fptr = __newline },
};


void __prntnum(i64 n)
{
	printf("%ld", n);
	fflush(stdout);
}

void _builtin_exit()
{
	exit(EXIT_SUCCESS);
}

void __newline()
{
	printf("\n");
}


static void *getcfn(char *name)
{
	size_t cnt = sizeof(c_fn_tbl) / sizeof(c_fn_tbl[0]);
	for (int i = 0; i < cnt; i++) {
		struct c_table_entry e = c_fn_tbl[i];
		if(strcmp(name, e.name) == 0) {
			return e.fptr;
		}
	}
	return NULL;
}


static void emit_instruction(struct genstate *s, struct instruction *i)
{
	emit_raw_data(s, sizeof(*i), i);
}

static void emit_push(struct genstate *s, struct operand o)
{
	struct instruction i = {
		.opcode = OPC_PUSH,
		.operands = { o },
	};
	emit_instruction(s, &i);
}

static void emit_pop(struct genstate *s, struct operand o)
{
	struct instruction i = {
		.opcode = OPC_POP,
		.operands = { o },
	};
	emit_instruction(s, &i);
}

static void emit_ret(struct genstate *s)
{
	struct instruction i = {
		.opcode = OPC_RET
	};
	emit_instruction(s, &i);
}

static void emit_pop_into_reg(struct genstate *s, enum regcode r)
{
	emit_pop(s, (struct operand) { .mode = MODE_REG, .reg = r });
}

static void emit_c_int_arg_opr(struct genstate *s, struct operand opr) {
	struct instruction ins = {
		.opcode = OPC_C_INT_ARG,
		.operands = { opr },
	};
	emit_instruction(s, &ins);
}

static void emit_c_int_arg_from_reg(struct genstate *s, enum regcode r)
{
	struct instruction i = {
		.opcode = OPC_C_INT_ARG,
		.operands = { { .mode = MODE_REG, .reg = r } }
	};
	emit_instruction(s, &i);
}

static void emit_c_int_arg_direct(struct genstate *s, i64 v)
{
	struct instruction i = {
		.opcode = OPC_C_INT_ARG,
		.operands = {{ .mode = MODE_DIRECT, .direct_value = (u64) v }},
	};
	emit_instruction(s, &i);
}

static void emit_c_call_void_direct(struct genstate *s, u64 data)
{
	struct instruction i = {
		.opcode = OPC_CALL_C_VOID,
		.operands = { { .mode = MODE_DIRECT, .direct_value = data } }
	};
	emit_instruction(s, &i);
}

static void emit_c_reset(struct genstate *s)
{
	struct instruction i = {
		.opcode = OPC_C_RESET,
	};
	emit_instruction(s, &i);
}

static void emit_form_call(struct genstate *s, struct scope *scope, struct formnode *fnp)
{
	char *id = fnp->identifier.identifier;
	void *pptr = getcfn(id);
	bool call_is_c =  pptr != NULL;

	if (call_is_c) {
		struct exprnode *arg = fnp->args;
		struct exprnode *arg_end = arg + arrlen(arg);
		for (; arg < arg_end; arg++) {
			switch (arg->type) {
			case EXPR_INTEGER:
				emit_c_int_arg_direct(s, arg->value.integer.value);
				break;
			case EXPR_IDENTIFIER:
			{
				struct decl_info *di = lookup_symbol(scope, arg->value.identifier.identifier);
				if (di == NULL) {
					errlocv_abort(arg->location,  "No reference found for %s", arg->value.identifier.identifier);
				}

				size_t o = hmget(s->offset_tbl, di->id);
				if (o != NOT_FOUND) {
					i64 *iv = (i64 *)(s->outbuf+o);
					emit_c_int_arg_direct(s, *iv);
				} else {
					u64 offset = offset_of_local_var(s, di->id);
					printf("offset %lu \n", offset);
					fflush(stdout);
					struct operand opr = {
						.mode = MODE_MEM,
						.reg = REG_SP,
						.offset = offset,
					};
					emit_c_int_arg_opr(s, opr);
					fprintf(stderr, "memes\n");
					fflush(stderr);
				}

				break;
			}					
			break;
			case EXPR_FORM:
				emit_form_call(s, scope, &arg->value.form);
				emit_pop_into_reg(s, REG_0);
				emit_c_int_arg_from_reg(s, REG_0);
				break;
			default:
				errloc_abort(arg->location, "Unexpected expression type");
			}
		}
		emit_c_call_void_direct(s, (u64)pptr);
		emit_c_reset(s);
	} else if (strlen(id) == 1 && *id == '+') {
		struct exprnode *arg = fnp->args;
		/* generate: add r1 r0 ; (r0 is destination) */
		struct operand a = {}, b = {};
		struct instruction ins;

		a.mode = MODE_DIRECT;
		a.direct_value = 0;
		b.reg = REG_0;
		b.mode = MODE_REG;
		ins = (struct instruction) {
			.opcode = OPC_MOV,
			.operands = { a, b },
		};
		emit_instruction(s, &ins);
		switch (arg->type) {
		case EXPR_INTEGER:
			a.direct_value = arg->value.integer.value;
			break;
		case EXPR_IDENTIFIER:
		{
			size_t o = hmget(s->offset_tbl,
			                 lookup_symbol(scope, arg->value.identifier.identifier)->id);

			if (o == NOT_FOUND)
				errlocv_abort(arg->location, "Could not find identifier %s", arg->value.identifier.identifier);

			a.direct_value = *(i64 *)(s->outbuf + o);
			break;
		}
		}
		b.reg = REG_0;
		b.mode = MODE_REG;
		ins = (struct instruction) {
			.opcode = OPC_ADD_INT,
			.operands = { a, b }
		};
		emit_instruction(s, &ins);
		arg++;

		switch (arg->type) {
		case EXPR_INTEGER:
			a.direct_value = arg->value.integer.value;
			break;
		case EXPR_IDENTIFIER:
		{
			size_t o = hmget(s->offset_tbl,
			                 lookup_symbol(scope, arg->value.identifier.identifier)->id);
			a.direct_value = *(i64 *)(s->outbuf + o);
			break;
		}
		}

		ins = (struct instruction) {
			.opcode = OPC_ADD_INT,
			.operands = { a, b }
		};
		emit_instruction(s, &ins);
		emit_push(s, b);

		return;
	} else {
		size_t uo = hmget(s->offset_tbl,
		                  lookup_symbol(scope, fnp->identifier.identifier)->id);

		if (uo == NOT_FOUND)
			errlocv_abort(fnp->location,
				      "Could not find procedure %s",
				      fnp->identifier.identifier);

		i64 o = (i64)uo;
		i64 now = arrlen(s->outbuf);
		i64 relative = o - now;
		struct operand opr = {
			.mode = MODE_DIRECT,
			.direct_value = relative,
		};
		struct instruction ins = {
			.opcode = OPC_CALL,
			.operands = { opr },
		};
		emit_instruction(s, &ins);
	}
}

static u64 offset_of_local_var(struct genstate *s, decl_id id)
{
	for (int i = 0; i < arrlen(s->local_var_offsets); ++i) {
		struct local_var_offset *vo = &s->local_var_offsets[i];
		if (vo->id == id) {
			return vo->offset;
		}
	}
	fprintf(stderr,
		"Fatal error tried to look up local var id %lu "
		"but it does not exist\n",
		id);
	exit(EXIT_FAILURE);
}

static void emit_var_initialize(
	struct genstate *s,
	struct scope *scope,
	struct varnode *vnp)
{
	struct instruction ins = {};
	struct operand opr = {};

	switch (vnp->value.type) {
	case EXPR_INTEGER:
		opr.mode = MODE_DIRECT;
		opr.direct_value = vnp->value.value.integer.value;
		break;
	case EXPR_IDENTIFIER:
		errloc_abort(vnp->location, "Can't initialize variable with identifier");
		break;
	case EXPR_FORM:
		errloc_abort(vnp->location, "Can't initialize variable with form");
		break;
	default:
		errloc_abort(vnp->location, "Can't initialize variable");
	}

	ins.opcode = OPC_MOV;
	ins.operands[0] = opr;

	opr.mode = MODE_MEM;
	opr.reg = REG_SP;
	decl_id vid = lookup_symbol(scope, vnp->identifier.identifier)->id;
	opr.offset = offset_of_local_var(s, vid);

	ins.operands[1] = opr;

	emit_instruction(s, &ins);
}

static void emit_block(struct genstate *s, struct blocknode *bnp)
{
	for (int i = 0; i < arrlen(bnp->exprs); ++i) {
		struct exprnode *expr = &bnp->exprs[i];
		struct scope *scope = hmget(scope_tbl, bnp);

		switch (expr->type) {
		case EXPR_FORM:
			emit_form_call(s, scope, &expr->value.form);
			break;
		case EXPR_BLOCK:
			emit_block(s, &expr->value.block);
			break;
		case EXPR_VAR:
			emit_var_initialize(s, scope, expr->value.var);
			break;
		default:
			printf("%d\n", expr->type);
			errloc_abort(expr->location, "Expected form or block");
		}
	}
}

static void emit_block_constants(struct genstate *s, struct blocknode *np)
{
	for (int i = 0; i < arrlen(np->defs); ++i) {
		emit_def(s, hmget(scope_tbl, np), &np->defs[i]);
	}

	for (int i = 0; i < arrlen(np->exprs); ++i) {
		struct exprnode *ep = &np->exprs[i];
		if (ep->type == EXPR_BLOCK)
			emit_block_constants(s, &ep->value.block);
	}

}

static void emit_enter(struct genstate *s)
{
	struct operand opr = {
		.mode = MODE_DIRECT,
		.direct_value = s->stackframe_size,
	};
	printf("stackframe size is %lu\n", s->stackframe_size);
	fflush(stdout);
	struct instruction ins = {
		.opcode = OPC_ENTER,
		.operands = {
			opr
		}
	};

	emit_instruction(s, &ins);
}

static void emit_leave(struct genstate *s)
{
	struct instruction ins = {
		.opcode = OPC_LEAVE,
	};

	emit_instruction(s, &ins);
}

static void emit_proc(struct genstate *s, struct procnode *pnp)
{
	struct scope *scope = hmget(scope_tbl, &pnp->block);
	emit_enter(s);
	for (int i = 0; i < arrlen(pnp->block.exprs); ++i) {
		struct	exprnode *expr = &pnp->block.exprs[i];

		switch (expr->type) {
		case EXPR_FORM:
			emit_form_call(s, hmget(scope_tbl, &pnp->block), &expr->value.form);
			break;
		case EXPR_BLOCK:
			emit_block(s, &expr->value.block);
			break;
		case EXPR_VAR:
			emit_var_initialize(s, scope, expr->value.var);
			break;
		default:
			errloc_abort(expr->location, "Expected form or block");
		}
	}

	emit_leave(s);
	emit_ret(s);
}

static void emit_raw_data(struct genstate *s, size_t sz, void *d)
{
	u8 *b = d;
	u8 *bend = b + sz;
	for (; b < bend; b++) {
		arrput(s->outbuf, *b);
	}
}

static void emit_integer(struct genstate *s, i64 integer)
{
	emit_raw_data(s, sizeof(integer), &integer);
}

static void set_block_var_offsets(struct genstate *s, struct blocknode *bnp)
{
	u64 offset = 0;

	for (int i = 0; i < arrlen(bnp->vars); ++i) {
		struct varnode *vnp = &bnp->vars[i];

		/* @TODO: when we have types other than ints fix here */
		u64 size = sizeof(i64);

		s->stackframe_size += size;

		struct scope *scope = hmget(scope_tbl, bnp);
		char *ident = vnp->identifier.identifier;
		struct decl_info *dip = lookup_symbol(scope, ident);
		struct local_var_offset lvo = {
			.id = dip->id,
			.offset = offset,
		};
		arrput(s->local_var_offsets, lvo);
		offset += size;

	}

	for (int i = 0; i < arrlen(bnp->exprs); ++i) {
		struct exprnode *enp = &bnp->exprs[i];
		if (enp->type == EXPR_BLOCK) {
			set_block_var_offsets(s, &enp->value.block);
		}
	}
}

static void set_proc_var_offsets(struct genstate *s, struct procnode *pnp)
{
	set_block_var_offsets(s, &pnp->block);
}

static void reset_proc_var_offsets(struct genstate *s)
{
	arrsetlen(s->local_var_offsets, 0);
	s->stackframe_size = 0;
}

static void emit_def(struct genstate *s, struct scope *scope, struct defnode *dnp)
{
	char *ident = dnp->identifier.identifier;

	size_t offs;
	struct exprnode *vp = &dnp->value;
	struct intnode *inp;
	struct procnode *pnp;
	/* @TODO: emit for identifiers */
	/* @TODO: emit for forms       */
	struct decl_info *dip = lookup_symbol(scope, ident);
	/* printf("%s has id %lu in emit def\n", ident, dip->id); */

	switch (vp->type) {
	case EXPR_INTEGER:
		offs = arrlen(s->outbuf);
		hmput(s->offset_tbl, dip->id, offs);
		/* printf("emitting %s at offset %lu\n", ident, offs); */
		inp = &vp->value.integer;
		emit_integer(s, inp->value);
		break;
	case EXPR_PROC:
		pnp = &vp->value.proc;
		emit_block_constants(s, &pnp->block);
		offs = arrlen(s->outbuf);
		hmput(s->offset_tbl, dip->id, offs);
		/* printf("emitting %s at offset %lu\n", ident, offs); */
		set_proc_var_offsets(s, pnp);
		emit_proc(s, pnp);
		reset_proc_var_offsets(s);
		break;
	}
}

static void emit_toplevel(struct genstate *gst, struct toplevelnode *tlnp)
{
	struct defnode *dnp = tlnp->definitions;
	struct defnode *dnpend = dnp + arrlen(dnp);
	for (; dnp < dnpend; dnp++) {
		emit_def(gst, hmget(scope_tbl, tlnp), dnp);
	}
}

static struct genstate emit_bytecode(struct toplevelnode *tlnp)
{
	struct genstate gst = {};

	arrsetcap(gst.outbuf, 1024*1024*8);
	hmdefault(gst.offset_tbl, NOT_FOUND);
	arrsetcap(gst.local_var_offsets, 64);

	emit_toplevel(&gst, tlnp);

	return gst;
}

static size_t emit_entry_point(struct genstate *gsp, size_t main_offset)
{
	size_t res = arrlen(gsp->outbuf);

	i64 now = (i64)res;
	i64 nowToMain = main_offset - now;

	struct instruction ins = {
		.opcode = OPC_CALL,
	};
	struct operand *opr = &ins.operands[0];
	opr->mode = MODE_DIRECT;
	opr->direct_value = nowToMain;

	emit_instruction(gsp, &ins);

	opr->direct_value = (u64)getcfn("exit");
	ins.opcode = OPC_CALL_C_VOID;

	emit_instruction(gsp, &ins);

	return res;
}
