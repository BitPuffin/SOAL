
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

#define SOURCE_BUFSIZE 8192000
char srcbuf[SOURCE_BUFSIZE];
int main()
{
	init();
	{	/* read source */
		FILE* testfile = fopen("test.soal", "r");
		size_t readcount = fread(srcbuf, 1, SOURCE_BUFSIZE, testfile);
		if(readcount >= SOURCE_BUFSIZE-1) {
			fprintf(stderr, "well we don't support a source file this big yet\n");
			fclose(testfile); /* :( */
			return EXIT_FAILURE;
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
