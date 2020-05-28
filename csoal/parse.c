/** Lexer section **/

#define delimiters "()[]{}"
#define token_chars "(){}[]\"\\':"

enum lex_mode {
	LX_MODE_NORMAL,
	LX_MODE_STRING,
	LX_MODE_ERROR,
	LX_MODE_DONE,
};

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
	TOK_COLON        = ':',
	
	TOK_IDENTIFIER   = 256,
	TOK_INTEGER,
};


struct token {
	struct srcloc location;
	enum token_type type;
	union {
		char *identifier;
		int integer;
	} value;
};

struct lex_state {
	struct srcloc location;
	char const* cursor;
	char const* token_end;
	size_t token_len;
	struct token token; /* current */
	enum lex_mode mode;
};

static bool            	 is_terminator(char c);
static char const      	*find_terminator(char const* str);
static struct lex_state	 initlex(char const* str, char const* srcpath);
static void            	 scan_to_non_whitespace(struct lex_state* state);
static char const      	*lex_into_token(struct token *token, char const *str);
static struct token    	*eat_token(struct lex_state* state);
static struct token    	*eat_token(struct lex_state* state);


static bool is_terminator(char c)
{
	return strchr(delimiters, c) != NULL || isspace(c);
}

/* return pointer to terminator or null if none was found */
static char const* find_terminator(char const* str)
{
	for (char const* it = str; *it != '\0'; ++it) {
		if(is_terminator(*it))
			return it;
	}
	return NULL;
}

static struct lex_state initlex(char const* str, char const* srcpath)
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

static char const* lex_into_token(struct token *token, char const *str)
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
		size_t sz = end - str;
		/* @TODO the following malloc is likely exensive */
		char* intstr = malloc(sizeof(char) * (sz + 1));
		strncpy(intstr, str, sz);
		intstr[sz] = '\0';
		token->value.integer = atoi(intstr);
	} else {
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

static struct token* eat_token(struct lex_state* state)
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
/* bool peek_token(struct lex_state *state, struct token *next) */
/* { */
/* 	/\* skip whitespace ugh *\/ */
/* 	char const* it = state->token_end; */
/* 	for(; *it != '\0'; ++it) { */
/* 		if (!isspace(*it)) */
/* 			break; */
/* 	} */
/* 	if(*state->token_end == '\0') { */
/* 		return false; */
/* 	} */
/* 	char const *ignore = lex_into_token(next, it); */
/* 	return true; */
/* } */

/** Parser section **/

#define open_parens "([{"
#define closing_parens ")]}"
#define matching_parens "()[]{}"

enum keywords {
	KW_PUB,
	KW_DEF,
	KW_VAR,
	KW_PROC,
	KW_RET,
};

static const char *const keyword_strs[] = {
	"pub",
	"def",
	"var",
	"proc",
	"return",
};

struct parser_state {
	struct lex_state lxstate;
	char *parenstack;
	/* struct node *nodestack; */
};

struct parser_state_snapshot {
	struct parser_state before;
	size_t paren_count_before;
};


static bool	 iskw(char const *ident);
static bool	 consume_char_tok(struct parser_state *ps, char c);
static bool	 is_open_paren(char c);
static bool	 is_closing_paren(char c);
static char	 get_matching_open_paren(char p);
static struct parser_state_snapshot
           	 make_parser_snapshot(struct parser_state *ps);
static void	 restore_parser_state(
           		struct parser_state *ps,
           		struct parser_state_snapshot *snap);
static bool	 consume_paren(struct parser_state *ps, char par);
static bool	 consume_kw(struct parser_state *ps, enum keywords kw);
static bool	 consume_iden(struct parser_state *ps, struct identnode *out);
static bool	 consume_integer(struct parser_state *ps, struct intnode *integer);
static bool	 consume_paramlist(struct parser_state *ps, struct argnode **out);
static void	 consume_block_inner(struct parser_state *ps, struct blocknode *bnp);
static bool	 consume_proc(struct parser_state *ps, struct procnode *proc);
static bool	 consume_form(struct parser_state *ps, struct formnode *form);
static bool	 consume_block(struct parser_state *ps, struct blocknode *bnp);
static bool	 consume_exprnode(struct parser_state *ps, struct exprnode *out);
static bool	 consume_def(struct parser_state *ps, struct defnode *out);
static bool	 consume_type_annotation(struct parser_state *psp, struct type_annotation_node *res);
static bool	 consume_type_annotation(struct parser_state *psp, struct type_annotation_node *res);
static bool	 consume_var(struct parser_state *psp, struct varnode *res);
static struct toplevelnode
           	 parse_toplevel(struct parser_state *ps);
static struct toplevelnode
           	 parse(char const *str, char const *fname);

static bool iskw(char const *ident)
{
	for(int i = 0; i < ARRLEN(keyword_strs); ++i) {
		if(strcmp(keyword_strs[i], ident) == 0) {
			return true;
		}
	}

	return false;
}

static bool consume_char_tok(struct parser_state *ps, char c)
{
	if(ps->lxstate.token.type == c) {
		eat_token(&ps->lxstate);
		return true;
	}
	return false;
}

static bool is_open_paren(char c)
{
	return strchr(open_parens, c) != NULL;
}

static bool is_closing_paren(char c)
{
	return strchr(closing_parens, c) != NULL;
}

static char get_matching_open_paren(char p)
{
	return *(strchr(matching_parens, p) - 1);
}

static struct parser_state_snapshot
make_parser_snapshot(struct parser_state *ps)
{
	struct parser_state_snapshot snap = {
		.before = *ps,
		.paren_count_before = arrlen(ps->parenstack),
	};
	return snap;
}

static void
restore_parser_snapshot(
	struct parser_state *ps,
	struct parser_state_snapshot *snap)
{
	snap->before.parenstack = ps->parenstack;
	*ps = snap->before;
	arrsetlen(ps->parenstack, snap->paren_count_before);

}

static bool consume_paren(struct parser_state *ps, char par)
{
	if (consume_char_tok(ps, par)) {
		if (is_closing_paren(par)) {
			char matching = get_matching_open_paren(par);
			char **pstack = &ps->parenstack;
			char lastpar = arrlast(*pstack);
			if(lastpar != matching) {
				struct srcloc loc = ps->lxstate.location;
				errlocv_abort(loc,
				              "Mismatching parenthesis, expected %c, got %c",
				              matching,
				              lastpar);
			}
			size_t len = arrlenu(*pstack);
			if(len == 0) {
				struct srcloc loc = ps->lxstate.location;
				errloc_abort(loc,
					     "No matching opening parenthesis found");
			}
			arrdel(*pstack, len - 1);
		} else {
			arrput(ps->parenstack, par);
		}
		return true;
	}
	return false;
}

static bool consume_kw(struct parser_state *ps, enum keywords kw)
{
	struct lex_state *ls = &ps->lxstate;
	bool pred = ls->token.type == TOK_IDENTIFIER;
	pred = pred && strcmp(keyword_strs[kw], ls->token.value.identifier) == 0;
	if (pred) {
		eat_token(ls);
		return true;
	} else {
		return false;
	}
}

static bool consume_iden(struct parser_state *ps, struct identnode *out)
{
	if (ps->lxstate.token.type != TOK_IDENTIFIER)
		return false;
	out->location = ps->lxstate.location;
	out->identifier = ps->lxstate.token.value.identifier;
	eat_token(&ps->lxstate);
	return true;
}

static bool consume_integer(struct parser_state *ps, struct intnode *integer)
{
	if (ps->lxstate.token.type != TOK_INTEGER) {
		return false;
	} else {
		integer->location = ps->lxstate.location;
		integer->value = ps->lxstate.token.value.integer;
		eat_token(&ps->lxstate);
		return true;
	}
}

static bool consume_paramlist(struct parser_state *ps, struct argnode **out)
{
	if (!consume_paren(ps, '('))
		return false;

	struct identnode in;
	struct type_annotation_node tn;
	while (consume_iden(ps, &in)) {
		if (!consume_type_annotation(ps, &tn))
			errlocv_abort(ps->lxstate.location,
			              "Expected type declaration for parameter %s",
			              in.identifier);
		printf("parameter name: %s, type name %s\n", in.identifier, tn.identifier.identifier);
		fflush(stdout);
	}

	if (!consume_paren(ps, ')'))
		errloc_abort(ps->lxstate.location,
		             "Expected closing parenthesis");

	return true;
}

static void consume_block_inner(struct parser_state *ps, struct blocknode *bnp)
{
	struct defnode def;
	struct exprnode expr;
	struct varnode var;

	for (;;) {
		if (consume_def(ps, &def)) {
			arrput(bnp->defs, def);
		} else if (consume_var(ps, &var)) {
			arrput(bnp->vars, var);
			expr = (struct exprnode) {
				.location = var.location,
				.type = EXPR_VAR,
				.value.var = &arrlast(bnp->vars),
			};
			arrput(bnp->exprs, expr);
		} else if (consume_exprnode(ps, &expr)) {
			arrput(bnp->exprs, expr);
		} else {
			break;
		}
	}
}	

static bool consume_proc(struct parser_state *ps, struct procnode *proc)
{
	struct parser_state_snapshot snap = make_parser_snapshot(ps);
	memset(proc, 0, sizeof(struct procnode));
	proc->location = ps->lxstate.location;

	if(!consume_paren(ps, '('))
		goto nope;
	if(!consume_kw(ps, KW_PROC))
		goto nope;
	if(!consume_iden(ps, &proc->returntype))
		goto nope;
	if(!consume_paramlist(ps, &proc->args))
		goto nope;

	consume_block_inner(ps, &proc->block);

	if (!consume_paren(ps, ')')) {
		struct srcloc loc = ps->lxstate.location;
		errloc_abort(loc, "Expected closing parenthesis");
	}

	return true;

nope:
	restore_parser_snapshot(ps, &snap);
	return false;
}

static bool consume_form(struct parser_state *ps, struct formnode *form)
{
	memset(form, 0, sizeof(struct formnode));
	form->location = ps->lxstate.location;
	if (!consume_paren(ps, '('))
		return false;
	if (!consume_iden(ps, &form->identifier))
		return false;
	
	struct exprnode arg = {};
	while(consume_exprnode(ps, &arg)) {
		arrput(form->args, arg);
	}

	if (!consume_paren(ps, ')')) {
		struct srcloc loc = ps->lxstate.location;
		errloc_abort(loc, "Expected closing parenthesis at %s:%lu,%lu\n");
	}

	return true;
}

static bool consume_block(struct parser_state *ps, struct blocknode *bnp)
{
	memset(bnp, 0, sizeof(struct blocknode));
	bnp->location = ps->lxstate.location;

	if (!consume_paren(ps, '{'))
		return false;

	consume_block_inner(ps, bnp);

	if (!consume_paren(ps, '}'))
		return false;

	return true;
}

static bool consume_ret(struct parser_state *ps, struct retnode *res)
{
	memset(res, 0, sizeof(struct retnode));
	res->location = ps->lxstate.location;

	if (!consume_paren(ps, '('))
		return false;

	if (!consume_kw(ps, KW_RET))
	    return false;

	if (consume_exprnode(ps, res->expr))
		if (res->expr->type == EXPR_RET)
			errloc_abort(res->expr->location, "Nested returns not allowed");

	if (!consume_paren(ps, ')'))
		errloc_abort(ps->lxstate.location, "Expected closing parenthesis");
	
	return true;
}

static bool consume_exprnode(struct parser_state *ps, struct exprnode *out)
{
	memset(out, 0, sizeof(struct exprnode));
	out->location = ps->lxstate.location;
	if (consume_integer(ps, &out->value.integer)) {
		out->type = EXPR_INTEGER;
		return true;
	} else if (consume_iden(ps, &out->value.identifier)) {
		out->type = EXPR_IDENTIFIER;
		return true;
	} else if (consume_proc(ps, &out->value.proc)) {
		out->type = EXPR_PROC;
		return true;
	} else if (consume_ret(ps, &out->value.ret)) {
		out->type = EXPR_RET;
		return true;
	} else if (consume_form(ps, &out->value.form)) {
		out->type = EXPR_FORM;
		return true;
	} else if (consume_block(ps, &out->value.block)) {
		out->type = EXPR_BLOCK;
		return true;
	}
	return false;
}

static bool consume_def(struct parser_state *ps, struct defnode *out)
{
	struct parser_state_snapshot snap = make_parser_snapshot(ps);

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
		errloc_abort(loc, "Expected identifier");
	}

	out->explicit_type = consume_iden(ps, &out->type);

	if(!consume_exprnode(ps, &out->value)) {
		struct srcloc loc = ps->lxstate.location;
		errloc_abort(loc, "Definition requires value expression");
	}

	if(!consume_paren(ps, ')')) {
		struct srcloc loc = ps->lxstate.location;
		errloc_abort(loc, "Expected closing parenthesis");
	}

	return true;
nope:
	restore_parser_snapshot(ps, &snap);
	return false;
}

static bool consume_type_annotation(
	struct parser_state *psp,
	struct type_annotation_node *res)
{
	struct parser_state_snapshot snap = make_parser_snapshot(psp);

	memset(res, 0, sizeof(struct type_annotation_node));
	res->location = psp->lxstate.location;

	if (!consume_paren(psp, '('))
		return false;

	if (!consume_char_tok(psp, ':'))
		goto nope;

	if (!consume_iden(psp, &res->identifier))
		errloc_abort(psp->lxstate.location, "Expected identifier for type");

	if (!consume_paren(psp, ')'))
		errloc_abort(psp->lxstate.location, "Expected closing parenthesis");

	return true;

nope:
	restore_parser_snapshot(psp, &snap);
	return false;
}

static bool consume_var(struct parser_state *psp, struct varnode *res)
{
	struct parser_state_snapshot snap = make_parser_snapshot(psp);
	memset(res, 0, sizeof(struct defnode));
	res->location = psp->lxstate.location;


	if(!consume_paren(psp, '('))
		goto nope;

	if(consume_kw(psp, KW_PUB))
		res->public = true;

	if(!consume_kw(psp, KW_VAR))
		goto nope;

	if(!consume_iden(psp, &res->identifier)) {
		struct srcloc loc = psp->lxstate.location;
		errloc_abort(loc, "Expected identifier");
	}

	res->explicit_type = consume_type_annotation(psp, &res->type);

	if(!consume_exprnode(psp, &res->value)) {
		struct srcloc loc = psp->lxstate.location;
		errloc_abort(loc, "Definition requires value expression");
	}

	if(!consume_paren(psp, ')')) {
		struct srcloc loc = psp->lxstate.location;
		errloc_abort(loc, "Expected closing parenthesis");
	}

	return true;

nope:
	restore_parser_snapshot(psp, &snap);
	return false;
}

static struct toplevelnode parse_toplevel(struct parser_state *ps)
{
	struct toplevelnode node = {};

	node.filename = ps->lxstate.location.path;

	struct defnode def = {};
	while (consume_def(ps, &def)) {
		arrput(node.definitions, def);
	}
	eat_token(&ps->lxstate);
	if (ps->lxstate.mode != LX_MODE_DONE) {
		struct srcloc loc = ps->lxstate.location;
		errloc_abort(loc, "Unexpected toplevel expression");
	}
	return node;
}

static struct toplevelnode parse(char const *str, char const *fname)
{
	struct parser_state ps = {};

	ps.lxstate = initlex(str, fname);
	eat_token(&ps.lxstate);
	return parse_toplevel(&ps);
}
