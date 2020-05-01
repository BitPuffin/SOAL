
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/stat.h>

#include "stb_ds.h"

#include "info.h"
#include "error.c"
#include "ints.h"
#include "ast.h"
#include "parse.c"
#include "symbol_resolution.c"
#include "typecheck.c"
#include "bytecodegen.c"
#include "unsafevm.c"
#include "init.c"
#include "bcdisass.c"

char *read_entire_file(char *path)
{
	char *buf;
	struct stat st;

	stat(path, &st);
	buf = malloc(sizeof(char) * st.st_size + 1);

	FILE* f = fopen(path, "r");
	size_t readcount = fread(buf, 1, st.st_size, f);
	if(readcount != st.st_size) {
		fprintf(stderr, "failed to read input file %s\n", path);
		fclose(f); /* :( */
		exit(EXIT_FAILURE);
	}
	buf[readcount] = '\0';
	fclose(f);
	return buf;
}

int main()
{
	init();

	char *srcbuf = read_entire_file("test.soal");
	struct toplevelnode n = parse(srcbuf, "test.soal");
	/*struct sym_table *stbl = collect_module_syms(&n);*/
	resolve_toplevel_symbols(&n);
	struct genstate gs = emit_bytecode(&n);
	size_t offset = shget(gs.offset_tbl, "do-my-stuff");
	struct instruction *mainstart = (struct instruction *)(gs.outbuf + offset);
	disass_proc(mainstart, mainstart + 18);
	run_program(&gs);
}


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
