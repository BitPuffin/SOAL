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

struct intnode {
	struct srcloc location;
	int value;
};

struct argnode {
	/* @TODO make args */
};


struct identnode {
	struct srcloc location;
	char const *identifier;
};

struct procnode {
	struct srcloc location;
	struct identnode returntype;
	struct argnode *args;
	struct exprnode *exprs;
};

struct formnode {
	struct srcloc location;
	struct identnode identifier;
	struct exprnode *args;
};

enum expr_type {
	EXPR_INTEGER,
	EXPR_IDENTIFIER,
	EXPR_PROC,
	EXPR_FORM,
};

struct exprnode {
	struct srcloc location;
	enum expr_type type;
	union {
		struct intnode integer;
		struct identnode identifier;
		struct procnode proc;
		struct formnode form;
	} value;
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
