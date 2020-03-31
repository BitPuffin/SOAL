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

struct {
	size_t child_count;
	struct node children[];
} form;

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
	int paren_ticker;
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

static void assign_token_boundraries(struct lex_state *state)
{
		state->token_end = find_terminator(state->cursor); /* @TODO factor out */
		state->token_len = state->token_end - state->cursor;
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

	struct token* tok = &(state->token);
	char firstc = *state->cursor;

	{	/* is it a single-char token */
		char* token_char = strchr(token_chars, firstc);
		if (token_char != NULL) {
			tok->type = *token_char;
			state->token_len = 1;
			state->token_end = state->cursor + 1;
			return;
		}
	}

	if (isdigit(firstc)) {
		assign_token_boundraries(state);
		state->token.type = TOK_INTEGER;
		/* @TODO set integer value */
	} else {
		/* it is an identifier */
		assign_token_boundraries(state);
		/* @TODO the following malloc is likely exensive */
		char* identstr = malloc(sizeof(char) * (state->token_len + 1)); 
		strncpy(identstr, state->cursor, state->token_len);
		identstr[state->token_len] = '\0';
		state->token.type = TOK_IDENTIFIER;
		state->token.value.identifier = identstr;
	}
}

#define SOURCE_BUFSIZE 102400
char srcbuf[SOURCE_BUFSIZE];
int main()
{
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

	struct lex_state lxstate = initlex(srcbuf, "test.soal");
	while(lxstate.mode == LX_MODE_NORMAL) {
		eat_token(&lxstate);
	}
}
