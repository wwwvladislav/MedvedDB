%{
#include "mdv_sql_lexer.h"
%}

%%

[0-9]+     { //yylval.val = atol(yytext);
             return(NUMBER);
           }
[0-9]+\.[0-9]+ {
             //sscanf(yytext,"%f",&yylval.val);
             return(NUMBER);
           }
"+"        return(PLUS);
"-"        return(MINUS);
"*"        return(MULT);
"/"        return(DIV);
"("        return(LB);
")"        return(RB);
\n         return(EOL);
[\t ]*     /* throw away whitespace */
.          { //yyerror("Illegal character"); 
             return(EOL);
           }
%%


