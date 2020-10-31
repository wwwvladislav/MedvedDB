/*
 * expr.y : A simple yacc expression parser
 *          Based on the Bison manual example. 
 */

%{
#include <stdio.h>
#include <math.h>

%}

%language "C"
%locations
%verbose

%define api.pure full
%define api.prefix {mdv_}
//%define parse.error detailed

%parse-param {int *nastiness} {int *randomness}

%union {
   float val;
}

%token NUMBER
%token PLUS MINUS MULT DIV
%token EOL
%token LB RB

%left  MINUS PLUS
%left  MULT DIV

%type  <val> exp NUMBER

%%
input   :
        %empty
        | input line
        ;

line    : EOL
        | exp EOL { printf("%g\n",$1);}

exp     : NUMBER                 { $$ = $1;        }
        | exp PLUS  exp          { $$ = $1 + $3;   }
        | exp MINUS exp          { $$ = $1 - $3;   }
        | exp MULT  exp          { $$ = $1 * $3;   }
        | exp DIV   exp          { $$ = $1 / $3;   }
        | MINUS  exp %prec MINUS { $$ = -$2;       }
        | LB exp RB              { $$ = $2;        }
        ;

%%
