%{
#include <stdio.h>
#include <stdlib.h>
int yylex(void);

%}

%union { char* ident; char* numlit; }
%start unit
%token DEF
%token <ident> IDENTIFIER
%token <numlit> NUM_LITERAL

%%

unit : declaration | unit declaration ;
declaration : const_var_declaration ;
const_var_declaration : '(' DEF IDENTIFIER IDENTIFIER literal ')' ;
literal: num_literal ;
num_literal : NUM_LITERAL ;

%%


int yyerror(char* err)
{
	fprintf(stderr, "%s\n", err);
	exit(1);
}

int yywrap()
{
	return 1;
}