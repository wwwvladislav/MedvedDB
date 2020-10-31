/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_MDV_HOME_VLAD_DEV_MEDVEDDB_MDV_CORE_STORAGE_MDV_SQL_LEXER_H_INCLUDED
# define YY_MDV_HOME_VLAD_DEV_MEDVEDDB_MDV_CORE_STORAGE_MDV_SQL_LEXER_H_INCLUDED
/* Debug traces.  */
#ifndef MDV_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define MDV_DEBUG 1
#  else
#   define MDV_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define MDV_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined MDV_DEBUG */
#if MDV_DEBUG
extern int mdv_debug;
#endif

/* Token type.  */
#ifndef MDV_TOKENTYPE
# define MDV_TOKENTYPE
  enum mdv_tokentype
  {
    NUMBER = 258,
    PLUS = 259,
    MINUS = 260,
    MULT = 261,
    DIV = 262,
    EOL = 263,
    LB = 264,
    RB = 265
  };
#endif

/* Value type.  */
#if ! defined MDV_STYPE && ! defined MDV_STYPE_IS_DECLARED
union MDV_STYPE
{
#line 22 "/home/vlad/Dev/MedvedDB/mdv_core/storage/mdv_sql.y"

   float val;

#line 80 "/home/vlad/Dev/MedvedDB/mdv_core/storage/mdv_sql_lexer.h"

};
typedef union MDV_STYPE MDV_STYPE;
# define MDV_STYPE_IS_TRIVIAL 1
# define MDV_STYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined MDV_LTYPE && ! defined MDV_LTYPE_IS_DECLARED
typedef struct MDV_LTYPE MDV_LTYPE;
struct MDV_LTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define MDV_LTYPE_IS_DECLARED 1
# define MDV_LTYPE_IS_TRIVIAL 1
#endif



int mdv_parse (int *nastiness, int *randomness);

#endif /* !YY_MDV_HOME_VLAD_DEV_MEDVEDDB_MDV_CORE_STORAGE_MDV_SQL_LEXER_H_INCLUDED  */
