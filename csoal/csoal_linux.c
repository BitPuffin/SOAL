
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/stat.h>

#include "stb_ds.h"

#include "info.h"
#include "ints.h"
#include "ast.h"
#include "parse.c"
#include "symbol_resolution.c"
#include "typecheck.c"
#include "bytecodegen.c"
#include "unsafevm.c"
#include "init.c"

int main()
{
	char *srcbuf;
	init();
	{	/* read source */
		struct stat st;
		stat("test.soal", &st);
		srcbuf = malloc(sizeof(char) * st.st_size + 1);
		FILE* testfile = fopen("test.soal", "r");
		size_t readcount = fread(srcbuf, 1, st.st_size, testfile);
		if(readcount != st.st_size) {
			fprintf(stderr, "failed to read input file\n");
			fclose(testfile); /* :( */
			exit(EXIT_FAILURE);
		}
		srcbuf[readcount] = '\0';
		fclose(testfile);
	}
	struct toplevelnode n = parse(srcbuf, "test.soal");
	/*struct sym_table *stbl = collect_module_syms(&n);*/
	resolve_toplevel_symbols(&n);
	struct genstate gs = emit_bytecode(&n);
	run_program(&gs);
}


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
