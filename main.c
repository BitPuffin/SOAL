#include <stdio.h>
#include "defs.h"
#include "parse.h"

extern void yyparse(void);

void sprint(char* a, char* b, ...)
{
}

int main(int argc, char** argv)
{
	u8 goal_version = 2;
	printf("Welcome to GOAL%d\n", goal_version);
	parse();
	return 0;
}
