#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

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

enum datum_t { INTEGER, SYMBOL };
struct datum {
	enum datum_t type;
	union { int integer; char const* symbol; } value;
};

enum node_type { FORM, DATUM };
struct node {
	enum node_type type;
	union { struct form* form; struct datum* datum; } value;
};

struct form {
	size_t child_count;
	struct node children[];
};

const char* token_chars = "(){}[]\"\\'";
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

struct srcloc {
	char const* path;
	size_t line;
	size_t column;
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


void parse(char const *str, char const *fname)
{
	struct lex_state lxstate = initlex(str, fname);
	struct token *t = &lxstate.token;
	while(lxstate.mode == LX_MODE_NORMAL) {
		eat_token(&lxstate);
		if (t->type == TOK_IDENTIFIER || t->type == TOK_INTEGER) {
			printf("%s:%lu,%lu: %s\n",
			       fname,
			       lxstate.location.line,
			       lxstate.location.column,
			       t->value.identifier);
		} else {
			printf("%s:%lu,%lu: %c\n",
			       fname,
			       lxstate.location.line,
			       lxstate.location.column,
			       (char)t->type);
		}
	}
}
