typedef size_t sym_id;

sym_id __next_sym_id;

sym_id gensymid()
{
	return __next_sym_id++;
}

struct sym_info {
	sym_id id;
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

		{
			sym_id id = gensymid();
			printf("%s.__symbol_id = %lu\n", k, id);
			struct sym_info info = {
				.id = id,
				.location = dnp->location,
				.identifier = k,
				.defnode = dnp,
			};
			shput(stbl, k, info);
		}
	}

	return stbl;
}

struct symdep {
	struct { char *key; char *value; } *syms;
};
struct symdep_tbl {
	sym_id key;
	struct symdep value;
};

struct	symdep collect_proc_deps(struct procnode *pnp);
struct	symdep collect_form_deps(struct formnode *fnp);
struct	symdep collect_expr_deps(struct exprnode *enp);

struct symdep collect_proc_deps(struct procnode *pnp)
{
	struct symdep dp = {};
	for (int i = 0; i < arrlen(pnp->exprs); ++i) {
		struct symdep exprd = collect_expr_deps(&pnp->exprs[i]);
		for (int j = 0; j < shlen(exprd.syms); ++j) {
			char *d = exprd.syms[j].value;
			shput(dp.syms, d, d);
		}
		shfree(exprd.syms);
	}
	return dp;
}

struct symdep collect_form_deps(struct formnode *fnp)
{
	struct symdep dp = {};
	shput(dp.syms, fnp->identifier.identifier, fnp->identifier.identifier);
	for (int i = 0; i < arrlen(fnp->args); i++) {
		struct symdep argd = collect_expr_deps(&fnp->args[i]);
		for (int j = 0; j < shlen(argd.syms); j++) {
			shput(dp.syms, argd.syms[j].value, argd.syms[j].value);
		}
		shfree(argd.syms);
	}
	return dp;
}

struct symdep collect_expr_deps(struct exprnode *enp)
{
	struct symdep dep = {};

	switch (enp->type) {
	case EXPR_IDENTIFIER:
		shput(dep.syms, enp->value.identifier.identifier, enp->value.identifier.identifier);
		break;
	case EXPR_PROC:
		dep = collect_proc_deps(&enp->value.proc);
		break;
	case EXPR_FORM:
		dep = collect_form_deps(&enp->value.form);
		break;
	}

	return dep;
}

void collect_symbol_dependencies(struct sym_table *stbl)
{
	struct symdep_tbl *sdtbl = NULL;
	for (int i = 0; i < shlen(stbl); i++) {
		struct sym_info *sip = &stbl[i].value;
		struct defnode *dnp = sip->defnode;
		struct exprnode *enp = &dnp->value;
		struct symdep dep = collect_expr_deps(enp);
		hmput(sdtbl, sip->id, dep);
	}

	for (int i = 0; i < hmlen(sdtbl); ++i) {
		sym_id sym = sdtbl[i].key;
		struct symdep dep = sdtbl[i].value;

		int len = shlen(dep.syms);
		if(len == 0)
			printf("symbol with id %lu has no dependencies.\n", sym);
		else
			printf("symbol with id %lu has dependencies: ", sym);
		for (int i = 0; i < len; ++i) {
			printf("%s", dep.syms[i].key);
			if (i + 1 != len)
				printf(" ");
		}
		printf("\n");
	}
	fflush(stdout);
}
