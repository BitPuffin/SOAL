%{
#include "y.tab.h"
extern int yyval;
%}


%%

\(                           return '(';
\)                           return ')';
[a-zA-Z]              return IDENTIFIER;         { yylval.ident = yytext; }
[0-9]                    return NUM_LITERAL;    { yylval.numlit = yytext; }
[ \t\n\r\v]            {;}

%%
