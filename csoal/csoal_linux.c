#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include <sys/stat.h>

#include <dyncall.h>

#include "stb_ds.h"

#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))

#include "ints.h"
#include "info.h"
#include "error.c"
#include "ast.h"
#include "parse.c"
#include "symbol_resolution.c"
#include "typecheck.c"
#include "init.c"
#include "uvm_bytecode.h"
#include "uvm_op_data.h"
#include "uvm_codegen.c"
#include "uvm.c"
#include "uvm_disass.c"

char *read_entire_file(char *path)
{
	char *buf;
	struct stat st;

	stat(path, &st);
	buf = malloc(sizeof(char) * st.st_size + 1);
	assert(buf != NULL);

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

	{
		struct scope *scope = hmget(scope_tbl, &n);
		size_t offset = hmget(gs.offset_tbl, lookup_symbol(scope, "main")->id);
		struct instruction *start = (struct instruction *)(gs.outbuf + offset);
		disass_proc(start, start + 42);
	}

	struct scope *scope = hmget(scope_tbl, &n);
	size_t main_offset = hmget(gs.offset_tbl, lookup_symbol(scope, "main")->id);
	if (main_offset == NOT_FOUND) {
		fprintf(stderr, "Could not run program (main could not be located)\n");
		exit(EXIT_FAILURE);
	}

	size_t entry_offset = emit_entry_point(&gs, main_offset);

	run_program(&gs, entry_offset);
}


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
