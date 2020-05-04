#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ints.h"

#include "unsafebytecode.h"

#define NOT_FOUND SIZE_MAX

struct c_table_entry {
	char *name;
	void *fptr;
};

void prntnum(i64 n)
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

struct c_table_entry c_fn_tbl[] = {
	{ .name = "print-number", .fptr = prntnum },
	{ .name = "exit", .fptr = _builtin_exit },
	{ .name = "newline", .fptr = __newline },
};

void *getcfn(char *name)
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

static void	emit_raw_data(struct genstate *s, size_t sz, void *d);

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

static void emit_form_call(struct genstate *s, struct formnode *fnp)
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
				size_t o = shget(s->offset_tbl,
						 arg->value.identifier.identifier);

				if (o == NOT_FOUND)
					errlocv_abort(arg->location, "Could not find identifier %s", arg->value.identifier.identifier);

				i64 *iv = (i64 *)(s->outbuf+o);
				emit_c_int_arg_direct(s, *iv);
				break;
			}					
			break;
			case EXPR_FORM:
				emit_form_call(s, &arg->value.form);
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
			size_t o = shget(s->offset_tbl, arg->value.identifier.identifier);

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
			size_t o = shget(s->offset_tbl, arg->value.identifier.identifier);
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
		size_t uo = shget(s->offset_tbl, fnp->identifier.identifier);

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

static void emit_block(struct genstate *s, struct blocknode *bnp)
{
	for (int i = 0; i < arrlen(bnp->exprs); ++i) {
		struct exprnode *expr = &bnp->exprs[i];

		switch (expr->type) {
		case EXPR_FORM:
			emit_form_call(s, &expr->value.form);
			break;
		case EXPR_BLOCK:
			emit_block(s, &expr->value.block);
			break;
		default:
			errloc_abort(expr->location, "Expected form or block");
		}
	}

}

static void emit_def(struct genstate *s, struct defnode *dnp);

static void emit_block_constants(struct genstate *s, struct blocknode *np)
{
	for (int i = 0; i < arrlen(np->defs); ++i) {
		emit_def(s, &np->defs[i]);
	}

	for (int i = 0; i < arrlen(np->exprs); ++i) {
		struct exprnode *ep = &np->exprs[i];
		if (ep->type == EXPR_BLOCK)
			emit_block_constants(s, &ep->value.block);
	}

}

static void emit_proc(struct genstate *s, struct procnode *pnp)
{
	for (int i = 0; i < arrlen(pnp->block.exprs); ++i) {
		struct	exprnode *expr = &pnp->block.exprs[i];

		switch (expr->type) {
		case EXPR_FORM:
			emit_form_call(s, &expr->value.form);
			break;
		case EXPR_BLOCK:
			emit_block(s, &expr->value.block);
			break;
		default:
			errloc_abort(expr->location, "Expected form or block");
		}
	}
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
	/* u8 *bend = (u8 *)&integer; */
	/* u8 *b = (u8 *)((&integer)+1); */
	/* for (;;) { */
	/* 	b--; */
	/* 	arrput(s->outbuf, *b); */
	/* 	if (b == bend) */
	/* 		break; */
	/* } */
}

static void emit_def(struct genstate *s, struct defnode *dnp)
{
	char *ident = dnp->identifier.identifier;

	size_t offs;
	struct exprnode *vp = &dnp->value;
	struct intnode *inp;
	struct procnode *pnp;
	/* @TODO: emit for identifiers */
	/* @TODO: emit for forms       */

	switch (vp->type) {
	case EXPR_INTEGER:
		offs = arrlen(s->outbuf);
		shput(s->offset_tbl, ident, offs);
		/* printf("emitting %s at offset %lu\n", ident, offs); */
		inp = &vp->value.integer;
		/* emit_raw_data(s, sizeof(inp->value), &inp->value); */
		emit_integer(s, inp->value);
		break;
	case EXPR_PROC:
		pnp = &vp->value.proc;
		emit_block_constants(s, &pnp->block);
		offs = arrlen(s->outbuf);
		shput(s->offset_tbl, ident, offs);
		/* printf("emitting %s at offset %lu\n", ident, offs); */
		emit_proc(s, pnp);
		break;
	}
}

static void emit_toplevel(struct genstate *gst, struct toplevelnode *tlnp)
{
	struct defnode *dnp = tlnp->definitions;
	struct defnode *dnpend = dnp + arrlen(dnp);
	for (; dnp < dnpend; dnp++) {
		emit_def(gst, dnp);
	}
}

struct genstate emit_bytecode(struct toplevelnode *tlnp)
{
	struct genstate gst = {};

	arrsetcap(gst.outbuf, 1024*1024*8);
	shdefault(gst.offset_tbl, NOT_FOUND);
	emit_toplevel(&gst, tlnp);

	return gst;
}
