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

struct c_table_entry c_fn_tbl[] = {
	{ .name = "print-number", prntnum }
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

static void emit_c_call_void_direct(struct genstate *s, u64 data)
{
	struct instruction i = {
		.opcode = OPC_CALL_C_VOID,
		.operands = { { .mode = MODE_DIRECT, .direct_value = data } }
	};
	emit_instruction(s, &i);
}

static void emit_form_call(struct genstate *s, struct formnode *fnp)
{
	char *id = fnp->identifier.identifier;
	struct exprnode *arg = fnp->args;
	if (strlen(id) == 1 && *id == '+') {
		/* generate: add r1 r0 ; (r0 is destination) */
		struct operand a = {}, b = {};

		a.mode = MODE_DIRECT;
		switch (arg->type) {
		case EXPR_INTEGER:
			a.direct_value = arg->value.integer.value;
			break;
		case EXPR_IDENTIFIER:
		{
			size_t o = shget(s->offset_tbl, arg->value.identifier.identifier);
			a.direct_value = *(s->outbuf + o);
			break;
		}
		}
		b.reg = REG_0;
		b.mode = MODE_REG;
		struct instruction ins = {
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
			a.direct_value = *(s->outbuf + o);
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
	}
	
}

static void emit_proc(struct genstate *s, struct procnode *pnp)
{
	struct	exprnode *expr = pnp->exprs;
	struct	exprnode *expr_end = expr + arrlen(expr);
	for (; expr < expr_end; ++expr) {
		if (expr->type != EXPR_FORM) {
			fprintf(stderr,
			        "Expected form at %s:(%ld, %ld)\n",
			        expr->location.path,
			        expr->location.line,
			        expr->location.column);
			exit(EXIT_FAILURE);
		}
		struct formnode *fnp = &expr->value.form;
		bool call_is_c = true;

		if (call_is_c) {
			struct exprnode *arg = fnp->args;
			struct exprnode *arg_end = arg + arrlen(arg);
			for (; arg < arg_end; arg++) {
				switch (arg->type) {
				case EXPR_INTEGER:
				case EXPR_IDENTIFIER:
				case EXPR_FORM:
					emit_form_call(s, &arg->value.form);
					break;
				default:
					fprintf(stderr,
						"Unexpected expression type at %s:(%ld, %ld)\n",
						arg->location.path,
						arg->location.line,
						arg->location.column);
					exit(EXIT_FAILURE);
				}
				emit_pop_into_reg(s, REG_0);
				emit_c_int_arg_from_reg(s, REG_0);
			}
			char *procname = fnp->identifier.identifier;
			void *pptr = getcfn(procname);
			emit_c_call_void_direct(s, (u64)pptr);
			
		} else {
			fprintf(stderr, "not implemented, bytecode fn call\n");
			exit(EXIT_FAILURE);
		}

		size_t o = shget(s->offset_tbl, fnp->identifier.identifier);
		if (o != NOT_FOUND) {
			/* Bytecode proc, but we can implement that later */
		} else {
			/* C proc (or fail) */
		}
	}
}

static void emit_raw_data(struct genstate *s, size_t sz, void *d)
{
	u8 *b = d;
	u8 *bend = b + sz;
	for (; b < bend; b++) {
		arrput(s->outbuf, *b);
	}
}

static void emit_def(struct genstate *s, struct defnode *dnp)
{
	char	*ident = dnp->identifier.identifier;

	{
		size_t offs = arrlen(s->outbuf);
		shput(s->offset_tbl, ident, offs);
	}

	struct exprnode *vp = &dnp->value;
	struct intnode *inp;
	struct procnode *pnp;
	/* @TODO: emit for identifiers */
	/* @TODO: emit for forms       */

	switch (vp->type) {
	case EXPR_INTEGER:
		inp = &vp->value.integer;
		emit_raw_data(s, sizeof(inp->value), &inp->value);
		break;
	case EXPR_PROC:
		pnp = &vp->value.proc;
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
