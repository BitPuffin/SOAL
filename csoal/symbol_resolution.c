typedef size_t sym_id;

struct sym_info {
	struct srcloc location;
	char *identifier;
	struct defnode *defnode;
};

struct sym_table { char *key; struct sym_info value; };
struct sym_table *collect_module_syms(struct toplevelnode *tlnp)
{
	struct sym_table *stbl = NULL;

	for (int i = 0; i < arrlen(tlnp->definitions); ++i) {
		struct defnode *dnp = &tlnp->definitions[i];
		char *k = dnp->identifier.identifier;
		
		{	/* check if it already exists */
			struct sym_info inf = shget(stbl, k);
			if (inf.identifier != NULL) {
				printf("Duplicate symbol %s at %s:(%lu, %lu)\n",
				       k,
				       dnp->location.path,
				       dnp->location.line,
				       dnp->location.column);
				printf("First discovered definition at %s:(%lu, %lu)\n",
				       inf.location.path,
				       inf.location.line,
				       inf.location.column);
				exit(EXIT_FAILURE);
			}
		}

		{	/* insert symbol */
			struct sym_info info = {
				.location = dnp->location,
				.identifier = k,
				.defnode = dnp,
			};
			shput(stbl, k, info);
		}
	}

	return stbl;
}

/* struct symdep { */
/* 	struct { char *key; char *value; } *syms; */
/* }; */
/* struct symdep_tbl { */
/* 	sym_id key; */
/* 	struct symdep value; */
/* }; */

/* struct	symdep collect_proc_deps(struct procnode *pnp); */
/* struct	symdep collect_form_deps(struct formnode *fnp); */
/* struct	symdep collect_expr_deps(struct exprnode *enp); */

/* struct symdep collect_proc_deps(struct procnode *pnp) */
/* { */
/* 	struct symdep dp = {}; */
/* 	for (int i = 0; i < arrlen(pnp->exprs); ++i) { */
/* 		struct symdep exprd = collect_expr_deps(&pnp->exprs[i]); */
/* 		for (int j = 0; j < shlen(exprd.syms); ++j) { */
/* 			char *d = exprd.syms[j].value; */
/* 			shput(dp.syms, d, d); */
/* 		} */
/* 		shfree(exprd.syms); */
/* 	} */
/* 	return dp; */
/* } */

/* struct symdep collect_form_deps(struct formnode *fnp) */
/* { */
/* 	struct symdep dp = {}; */
/* 	shput(dp.syms, fnp->identifier.identifier, fnp->identifier.identifier); */
/* 	for (int i = 0; i < arrlen(fnp->args); i++) { */
/* 		struct symdep argd = collect_expr_deps(&fnp->args[i]); */
/* 		for (int j = 0; j < shlen(argd.syms); j++) { */
/* 			shput(dp.syms, argd.syms[j].value, argd.syms[j].value); */
/* 		} */
/* 		shfree(argd.syms); */
/* 	} */
/* 	return dp; */
/* } */

/* struct symdep collect_expr_deps(struct exprnode *enp) */
/* { */
/* 	struct symdep dep = {}; */

/* 	switch (enp->type) { */
/* 	case EXPR_IDENTIFIER: */
/* 		shput(dep.syms, enp->value.identifier.identifier, enp->value.identifier.identifier); */
/* 		break; */
/* 	case EXPR_PROC: */
/* 		dep = collect_proc_deps(&enp->value.proc); */
/* 		break; */
/* 	case EXPR_FORM: */
/* 		dep = collect_form_deps(&enp->value.form); */
/* 		break; */
/* 	} */

/* 	return dep; */
/* } */

/* void collect_symbol_dependencies(struct sym_table *stbl) */
/* { */
/* 	struct symdep_tbl *sdtbl = NULL; */
/* 	for (int i = 0; i < shlen(stbl); i++) { */
/* 		struct sym_info *sip = &stbl[i].value; */
/* 		struct defnode *dnp = sip->defnode; */
/* 		struct exprnode *enp = &dnp->value; */
/* 		struct symdep dep = collect_expr_deps(enp); */
/* 		hmput(sdtbl, sip->id, dep); */
/* 	} */

/* 	for (int i = 0; i < hmlen(sdtbl); ++i) { */
/* 		sym_id sym = sdtbl[i].key; */
/* 		struct symdep dep = sdtbl[i].value; */

/* 		int len = shlen(dep.syms); */
/* 		if(len == 0) */
/* 			printf("symbol with id %lu has no dependencies.", sym); */
/* 		else */
/* 			printf("symbol with id %lu has dependencies: ", sym); */
/* 		for (int i = 0; i < len; ++i) { */
/* 			printf("%s", dep.syms[i].key); */
/* 			if (i + 1 != len) */
/* 				printf(" "); */
/* 		} */
/* 		printf("\n"); */
/* 	} */
/* 	fflush(stdout); */
/* } */

struct scope {
	struct sym_table *sym_tbl;
	struct scope *parent;
};

struct scope *push_scope(struct scope *parent)
{
	struct scope *sp = malloc(sizeof(struct scope));
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

/* void resolve_proc_dependencies(struct procnode *pnp, struct scope *decl_scope) */
/* { */
/* 	struct scope *sp = push_scope(decl_scope); */

/* 	struct symdep depp = collect_proc_deps(pnp); */
/* 	size_t end = shlen(depp.syms); */
/* 	for (int i = 0; i < end; ++i) { */
/* 		char *dep = NULL; */
/* 		sym_id id = lookup_symbol(sp, dep); */
/* 		if(id == NOT_FOUND) { */
/* 			/\* @TODO: can't report location *\/ */
/* 			fprintf(stderr, */
/* 			        "Could not resolve symbol %s", */
/* 			        dep); */
/* 			exit(EXIT_FAILURE); */
/* 		} */
/* 	} */

/* 	pop_scope(sp); */
/* } */

struct scope *_default_scope;

bool scope_has_same_symbol(struct scope *sp, char *sym)
{
	return shget(sp->sym_tbl, sym).identifier != NULL;
}

void resolve_form_syms(struct scope *sp, struct formnode *fnp);
void resolve_proc_syms(struct scope *scp, struct procnode *pnp)
{
	struct scope *sp = push_scope(scp);

	/* @TODO: add all the proc level defs to the scope */

	for (int i = 0; i < arrlen(pnp->exprs); ++i) {
		struct exprnode *enp = &pnp->exprs[i];

		if (enp->type == EXPR_FORM) {
			resolve_form_syms(sp, &enp->value.form);
		}
	}
}

void resolve_form_syms(struct scope *sp, struct formnode *fnp)
{
	if (lookup_symbol(sp, fnp->identifier.identifier) == NULL) {
		fprintf(stderr,
			"Could not locate reference to %s at %s:(%lu, %lu)\n",
			fnp->identifier.identifier,
			fnp->location.path,
			fnp->location.line,
			fnp->location.column);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < arrlen(fnp->args); ++i) {
		struct exprnode *enp = &fnp->args[i];
		switch (enp->type) {
		case EXPR_IDENTIFIER:
			if (lookup_symbol(sp, enp->value.identifier.identifier) == NULL) {
				fprintf(stderr,
					"Could not locate reference to %s at %s:(%lu, %lu)\n",
					enp->value.identifier.identifier,
					enp->location.path,
					enp->location.line,
					enp->location.column);
				exit(EXIT_FAILURE);
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
			fprintf(stderr,
			        "Duplicate definition of %s at %s:(%lu, %lu)\n",
			        dnp->identifier.identifier,
			        dnp->location.path,
			        dnp->location.line,
			        dnp->location.column);
			exit(EXIT_FAILURE);
		}

		scope_insert_def(scope, dnp);
	}

	for (int i = 0; i < arrlen(tlnp->definitions); ++i) {
		struct defnode *dnp = &tlnp->definitions[i];
		struct exprnode *enp = &dnp->value;

		switch (enp->type) {
		case EXPR_IDENTIFIER:
			if (lookup_symbol(scope, enp->value.identifier.identifier) == NULL) {
				fprintf(stderr,
					"Could not locate reference to %s at %s:(%lu, %lu)\n",
				        enp->value.identifier.identifier,
				        enp->value.identifier.location.path,
				        enp->value.identifier.location.line,
				        enp->value.identifier.location.column);
				exit(EXIT_FAILURE);
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
	memset(_default_scope, 0, sizeof(struct scope));

	_scope_builtin("+");
	_scope_builtin("-");
	_scope_builtin("/");
	_scope_builtin("print-number");
}

void symres_init()
{
	init_default_scope();
}
