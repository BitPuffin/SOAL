#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>

#include "stb_ds.h"

char const* delimiters = "()[]{}";
bool is_terminator(char c)
{
	return strchr(delimiters, c) != NULL || isspace(c);
}

/* return pointer to terminator or null if none was found */
char const* find_terminator(char const* str)
{
	for(char const* it = str; *it != '\0'; ++it) {
		if(is_terminator(*it))
			return it;
	}
	return NULL;
}

struct srcloc {
	char const* path;
	size_t line;
	size_t column;
};

const char* token_chars  = "(){}[]\"\\'";
enum token_type {
	TOK_ERROR,
	TOK_PAREN_OPEN   = '(',
	TOK_PAREN_CLOSE  = ')',
	TOK_SQUARE_OPEN  = '[',
	TOK_SQUARE_CLOSE = ']',
	TOK_CURLY_OPEN   = '{',
	TOK_CURLY_CLOSE  = '}',
	TOK_QUOTE        = '\'',
	TOK_DOUBLE_QUOTE = '"',
	
	TOK_IDENTIFIER   = 256,
	TOK_INTEGER,
};

struct token {
	struct srcloc location;
	enum token_type type;
	union {
		char const* identifier;
		int integer;
	} value;
};

enum lex_mode {
	LX_MODE_NORMAL,
	LX_MODE_STRING,
	LX_MODE_ERROR,
	LX_MODE_DONE,
};

struct lex_state {
	struct srcloc location;
	char const* cursor;
	char const* token_end;
	size_t token_len;
	struct token token; /* current */
	enum lex_mode mode;
};

struct lex_state initlex(char const* str, char const* srcpath)
{
	struct lex_state state = {};
	state.cursor    = str;
	state.token_end = str;
	state.location.path = srcpath == NULL ? "(unknown-file.soal)" : srcpath;
	state.location.line = 1;
	return state;
}

static void scan_to_non_whitespace(struct lex_state* state)
{
	char const* it = state->cursor;
	for(; *it != '\0'; ++it) {
		if (!isspace(*it))
			break;
		else if (*it == '\n') { /* @TODO: handle \r\f  etc*/
			state->location.line++;
			state->location.column = 0;
		} else {
			state->location.column++;
		}
	}
	state->cursor = it;
}

char const* lex_into_token(struct token *token, char const *str)
{
	char const *end;
	char firstc = *str;

	{	/* is it a single-char token */
		char* token_char = strchr(token_chars, firstc);
		if (token_char != NULL) {
			token->type = *token_char;
			end = str + 1;
			return end;
		}
	}

	end = find_terminator(str);
	if (isdigit(firstc)) {
		token->type = TOK_INTEGER;
		/* @TODO set integer value */
	} /*else*/ {
		token->type = TOK_IDENTIFIER;
		/* it is an identifier */
		size_t sz = end - str;
		/* @TODO the following malloc is likely exensive */
		char* identstr = malloc(sizeof(char) * (sz + 1));
		strncpy(identstr, str, sz);
		identstr[sz] = '\0';
		token->value.identifier = identstr;
	}
	return end;
}

struct token* eat_token(struct lex_state* state)
{
	/* are we done? */
	if(*state->cursor == '\0') {
		state->mode = LX_MODE_DONE;
		return NULL;
	}

	/* move forward */
	state->cursor = state->token_end;
	state->location.column += state->token_len;
	scan_to_non_whitespace(state);

	struct token* tok = &state->token;

	state->token_end = lex_into_token(tok, state->cursor);
	state->token_len = state->token_end - state->cursor;
	tok->location = state->location;

	return tok;
}

/* implementing peek token may be a mistake because */
/* not sure a lisp syntax even needs it */
/* so it made eat_token more complicated */
bool peek_token(struct lex_state *state, struct token *next)
{
	/* skip whitespace ugh */
	char const* it = state->token_end;
	for(; *it != '\0'; ++it) {
		if (!isspace(*it))
			break;
	}
	if(*state->token_end == '\0') {
		return false;
	}
	char const *ignore = lex_into_token(next, it);
	return true;
}


const char* open_parens  = "([{";
bool is_open_paren(char c)
{
	return strchr(open_parens, c) != NULL;
}

const char* closing_parens = ")]}";
bool is_closing_paren(char c)
{
	return strchr(closing_parens, c) != NULL;
}

const char* matching_parens = "()[]{}";
char get_matching_open_paren(char p)
{
	return *(strchr(matching_parens, p) - 1);
}

enum datum_t { INTEGER, IDENTIFIER };
struct datum {
	enum datum_t type;
	union { int integer; char const* identifier; } value;
};


enum node_type { DEF_FORM, PROC_FORM , FORM, DATUM };
struct node {
	struct srcloc location;
	enum node_type type;
	union { struct form* form; struct datum* datum; } value;
};

struct parser_state {
	struct lex_state lxstate;
	char *parenstack;
	struct node *nodestack;
};

#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))

enum keywords {
	KW_PUB,
	KW_DEF,
	KW_PROC,
};
char const* keyword_strs[] = {
	"pub",
	"def",
	"proc"
};
bool iskw(char const *ident)
{
	for(int i = 0; i < ARRLEN(keyword_strs); ++i) {
		if(strcmp(keyword_strs[i], ident) == 0) {
			return true;
		}
	}

	return false;
}

bool consume_char_tok(struct parser_state *ps, char c)
{
	if(ps->lxstate.token.type == c) {
		eat_token(&ps->lxstate);
		return true;
	}
	return false;
}

bool consume_paren(struct parser_state *ps, char par) {
	if (consume_char_tok(ps, par)) {
		arrput(ps->parenstack, par);
		return true;
	}
	return false;
}

bool consume_kw(struct parser_state *ps, enum keywords kw)
{
	struct lex_state *ls = &ps->lxstate;
	bool p = ls->token.type == TOK_IDENTIFIER;
	p = p && strcmp(keyword_strs[kw], ls->token.value.identifier) == 0;
	if (p) {
		eat_token(ls);
		return true;
	} else {
		return false;
	}
}


struct intnode {
	struct srcloc location;
	int value;
};

struct argnode {
	/* @TODO make args */
};

struct procnode {
	
};

struct identnode {
	struct srcloc location;
	char const *identifier;
};

struct exprnode;
struct formnode {
	struct srcloc location;
	struct identnode identifier;
	struct exprnode *args;
};

enum expr_type {
	EXPR_INTEGER,
	EXPR_PROC,
	EXPR_FORM,
};
struct exprnode {
	struct srcloc location;
	enum expr_type type;
	union {
		struct intnode integer;
		struct procnode proc;
		struct formnode call;
	} value;
};


bool consume_iden(struct parser_state *ps, struct identnode *out)
{
	if (ps->lxstate.token.type != TOK_IDENTIFIER)
		return false;

	out->location = ps->lxstate.location;
	out->identifier = ps->lxstate.token.value.identifier;
	eat_token(&ps->lxstate);
	return true;
}

bool consume_exprnode(struct parser_state *ps, struct exprnode *out)
{
	
}

struct defnode {
	struct srcloc location;
	bool public;
	bool explicit_type;
	struct identnode type; /* only valid if explict_type is true */
	struct identnode identifier;
	struct exprnode value;
};

bool consume_def(struct parser_state *ps, struct defnode *out)
{
	struct parser_state before = *ps;
	memset(out, 0, sizeof(struct defnode));
	out->location = ps->lxstate.location;

	if(!consume_paren(ps, '('))
		goto nope;

	if(consume_kw(ps, KW_PUB))
		out->public = true;

	if(!consume_kw(ps, KW_DEF))
		goto nope;

	if(!consume_iden(ps, &out->identifier)) {
		struct srcloc loc = ps->lxstate.location;
		fprintf(stderr,
		        "Expected identifier at %s:%lu,%lu\n",
		        loc.path,
		        loc.line,
		        loc.column);
		exit(EXIT_FAILURE);
	}

	out->explicit_type = consume_iden(ps, &out->type);

	if(!consume_exprnode(ps, &out->value)) {
		struct srcloc loc = ps->lxstate.location;
		fprintf(stderr,
		        "Definition requires value at %s:%lu,%lu\n",
		        loc.path,
		        loc.line,
		        loc.column);
		exit(EXIT_FAILURE);
	}

	if(!consume_paren(ps, ')')) {
		struct srcloc loc = ps->lxstate.location;
		fprintf(stderr,
		        "expected closing parenthesis at %s:%lu,%lu\n",
		        loc.path,
		        loc.line,
		        loc.column);
		exit(EXIT_FAILURE);
	}

	return true;
nope:
	*ps = before;
	return false;
}

void parse_toplevel(struct lex_state *lxstate) {
}

void parse(char const *str, char const *fname)
{
	struct lex_state lxstate = initlex(str, fname);
	struct token *t = &lxstate.token;
	struct token next = {};
	while(lxstate.mode == LX_MODE_NORMAL) {
		eat_token(&lxstate);
		if (t->type == TOK_PAREN_OPEN) {
			eat_token(&lxstate);
			if(t->type == TOK_IDENTIFIER && iskw(t->value.identifier)) {
				 /* ignore if it exists for now */
				peek_token(&lxstate, &next);
				
			}
		}
	}
}
