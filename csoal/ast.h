struct intnode {
	struct srcloc location;
	i64 value;
};

struct identnode {
	struct srcloc location;
	char *identifier;
};

struct blocknode {
	struct srcloc location;
	struct exprnode *exprs;
	struct defnode *defs;
	struct varnode *vars;
};

struct procnode {
	struct srcloc location;
	struct identnode returntype;
	struct argnode *args;
	struct blocknode block;
};

struct formnode {
	struct srcloc location;
	struct identnode identifier;
	struct exprnode *args;
};

struct retnode {
	struct srcloc location;
	struct exprnode *expr;
};

enum expr_type {
	EXPR_INTEGER,
	EXPR_IDENTIFIER,
	EXPR_PROC,
	EXPR_FORM,
	EXPR_BLOCK,
	EXPR_VAR,
	EXPR_RET
};

struct type_annotation_node {
	struct srcloc location;
	struct identnode identifier;
};

struct exprnode {
	struct srcloc location;
	enum expr_type type;
	union {
		struct intnode integer;
		struct identnode identifier;
		struct procnode proc;
		struct formnode form;
		struct blocknode block;
		struct varnode *var;
		struct retnode ret;
	} value;
};

struct varnode {
	struct srcloc location;
	bool public;
	bool explicit_type;
	struct type_annotation_node type;
	struct identnode identifier;
	struct exprnode value;
};

struct defnode {
	struct srcloc location;
	bool public;
	bool explicit_type;
	struct identnode type; /* only valid if explict_type is true */
	struct identnode identifier;
	struct exprnode value;
};

struct toplevelnode {
	char const *filename;
	struct defnode *definitions;
};
