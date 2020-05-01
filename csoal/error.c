void errloc_abort(char *msg, struct srcloc loc)
{
	fprintf(stderr,
	        "%s at %s(%lu, %lu)\n",
	        msg,
	        loc.path,
	        loc.line,
	        loc.column);
	exit(EXIT_FAILURE);
}

#define errlocv_abort(loc, msg, ...)       \
	fprintf(stderr,                    \
	        msg " at %s(%lu, %lu)\n",  \
	        __VA_ARGS__,               \
	        loc.path,                  \
	        loc.line,                  \
	        loc.column);		   \
	exit(EXIT_FAILURE);
