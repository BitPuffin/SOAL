typedef size_t sym_id;

struct sym_info {
	struct srcloc location;
	char *identifier;
	struct defnode *defnode;
};

struct scope {
	struct sym_table *sym_tbl;
	struct scope *parent;
};

struct sym_table { char *key; struct sym_info value; };

struct scope *push_scope(struct scope *parent)
{
	struct scope *sp = malloc(sizeof(struct scope));
	assert(sp != NULL);
	memset(sp, 0, sizeof(struct scope));

	sp->parent = parent;
	return sp;
}

struct scope *pop_scope(struct scope *sp)
{
	struct scope *pp = sp->parent;
	free(sp);
	return pp;
}

void scope_insert_def(struct scope *sp, struct defnode *dnp)
{
	struct sym_info inf = {
		.location = dnp->location,
		.identifier = dnp->identifier.identifier,
		.defnode = dnp,
	};
	shput(sp->sym_tbl, inf.identifier, inf);
}

struct sym_info *lookup_symbol(struct scope *sp, char *dep)
{
	while(sp != NULL) {
		struct sym_info *info = &shget(sp->sym_tbl, dep);
		if(info->identifier != NULL) {
			return info;
		} else {
			sp = sp->parent;
		}
	}
	return NULL;
}

struct scope *_default_scope;

bool scope_has_same_symbol(struct scope *sp, char *sym)
{
	return shget(sp->sym_tbl, sym).identifier != NULL;
}

void resolve_form_syms(struct scope *sp, struct formnode *fnp);

void resolve_block_syms(struct scope *sp, struct blocknode *bnp)
{
	struct scope *nsp = push_scope(sp);

	for (int i = 0; i < arrlen(bnp->defs); ++i) {
		scope_insert_def(nsp, &bnp->defs[i]);
	}

	for (int i = 0; i < arrlen(bnp->exprs); ++i) {
		struct exprnode *enp = &bnp->exprs[i];

		if (enp->type == EXPR_FORM) {
			resolve_form_syms(nsp, &enp->value.form);
		} else if (enp->type == EXPR_BLOCK) {
			resolve_block_syms(nsp, &enp->value.block);
		}
	}
}

void resolve_proc_syms(struct scope *sp, struct procnode *pnp)
{
	resolve_block_syms(sp, &pnp->block);
}

void resolve_form_syms(struct scope *sp, struct formnode *fnp)
{
	if (lookup_symbol(sp, fnp->identifier.identifier) == NULL) {
		errlocv_abort(fnp->location,
		              "Could not locate reference to %s",
		              fnp->identifier.identifier);
	}

	for (int i = 0; i < arrlen(fnp->args); ++i) {
		struct exprnode *enp = &fnp->args[i];
		switch (enp->type) {
		case EXPR_IDENTIFIER:
			if (lookup_symbol(sp, enp->value.identifier.identifier) == NULL) {
				errlocv_abort(enp->location,
				              "could not locate reference to %s",
				              enp->value.identifier.identifier);
			}
			break;
		case EXPR_FORM:
			resolve_form_syms(sp, &enp->value.form);
			break;
		}
	}
}

void resolve_toplevel_symbols(struct toplevelnode *tlnp)
{
	struct scope *scope = push_scope(_default_scope);

	for (int i = 0; i < arrlen(tlnp->definitions); ++i) {
		struct defnode *dnp = &tlnp->definitions[i];

		if (scope_has_same_symbol(scope, dnp->identifier.identifier)) {
			errlocv_abort(dnp->location,
			              "Duplicate definition of %s",
			              dnp->identifier.identifier);
		}

		scope_insert_def(scope, dnp);
	}

	for (int i = 0; i < arrlen(tlnp->definitions); ++i) {
		struct defnode *dnp = &tlnp->definitions[i];
		struct exprnode *enp = &dnp->value;

		switch (enp->type) {
		case EXPR_IDENTIFIER:
			if (lookup_symbol(scope, enp->value.identifier.identifier) == NULL) {
				errlocv_abort(enp->location,
				              "Could not locate reference to %s",
				              enp->value.identifier.identifier);
			}
			break;
		case EXPR_PROC:
			resolve_proc_syms(scope, &enp->value.proc);
			break;
		case EXPR_FORM:
			resolve_form_syms(scope, &enp->value.form);
			break;
		}

		/* lookup_symbol() */
	}
}

void _scope_builtin(char *name)
{
	struct sym_info info = {};
	info = (struct sym_info) {
		.location = { .path = "<builtin>" },
		.identifier = name,
		.defnode = NULL
	};
	shput(_default_scope->sym_tbl, name, info);
}

void init_default_scope()
{
	_default_scope = malloc(sizeof(struct scope));
	assert(_default_scope != NULL);
	memset(_default_scope, 0, sizeof(struct scope));

	_scope_builtin("+");
	_scope_builtin("-");
	_scope_builtin("/");
	_scope_builtin("print-number");
	_scope_builtin("exit");
	_scope_builtin("newline");
}

void symres_init()
{
	init_default_scope();
}
