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
void collect_module_syms(struct toplevelnode *tlnp)
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
			struct sym_info info = {
				.id = id,
				.location = dnp->location,
				.identifier = k,
				.defnode = dnp,
			};
			shput(stbl, k, info);
		}
	}

	for (int i = 0; i < shlen(stbl); i++) {
		struct sym_info inf = stbl[i].value;
		printf("%s is defined at %s:(%lu, %lu)\n",
		       inf.identifier,
		       inf.location.path,
		       inf.location.line,
		       inf.location.column);
	}
}
	
