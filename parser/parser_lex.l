/* $Id$ */

/*
 *   Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *   NOVELL (All rights reserved)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, contact Novell, Inc.
 */

/* Definitions section */
/* %option main */

/* eliminates need to link with libfl */
%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#define _(s) gettext(s)

#include "parser.h"
#include "parser_yacc.h"

/* #define DEBUG */
#ifdef DEBUG
#define PDEBUG(fmt, args...) printf("Lexer: " fmt, ## args)
#else
#define PDEBUG(fmt, args...)	/* Do nothing */
#endif
#define NPDEBUG(fmt, args...)	/* Do nothing */

int current_lineno = 1;

%}

UP		"^"
OPEN_BRACE 	\{
CLOSE_BRACE 	\}
SLASH		\/
COLON		:
END_OF_RULE	[,]
SEPERATOR 	{UP}
RANGE		-
MODES 		([RrWwaLlMmk]|([Pp][Xx])|([Uu][Xx])|([Ii][Xx]))+
WS		[[:blank:]]
NUMBER		[[:digit:]]+
ID 		[^ \t\n"!,]|(,[^ \t\n"!])
POST_VAR_ID 	[^ =\+\t\n"!,]|(,[^ =\+\t\n"!])
IP		{NUMBER}\.{NUMBER}\.{NUMBER}\.{NUMBER}
ALLOWED_QUOTED_ID 	[^\0"]|\\\"
QUOTED_ID	\"{ALLOWED_QUOTED_ID}*\"

HAT		hat[ \t]+
KEYWORD         [[:alpha:]_]+
VARIABLE_NAME	[[:alpha:]][[:alnum:]_]*
SET_VAR_PREFIX  @
SET_VARIABLE	{SET_VAR_PREFIX}(\{{VARIABLE_NAME}\}|{VARIABLE_NAME})
BOOL_VARIABLE	$(\{{VARIABLE_NAME}\}|{VARIABLE_NAME})

PATHNAME	(\/|{SET_VARIABLE}{POST_VAR_ID}){ID}*
QPATHNAME	\"(\/|{SET_VAR_PREFIX})([^\0"]|\\\")*\"

IFACE		[[:alnum:]:]+

FLAGOPEN_PAREN 	\(
FLAGCLOSE_PAREN	\)
FLAGSEP		\,
EQUALS		=
ADD_ASSIGN	\+=

%x SUB_NAME
%x IP_MODE
%x IF_MODE
%x NETWORK_MODE
%x FLAGS_MODE
%x ASSIGN_MODE

%%

<SUB_NAME>{
	{ID}+	{
			  /* Ugh, this is a gross hack. I used to use
			   * {ID}+ to match all TOK_IDs, but that would
			   * also match TOK_MODE + TOK_END_OF_RULE
			   * without any spaces in between (because it's
			   * a longer match). So now, when I want to
			   * match any random string, I go into a
			   * seperate state. */
			yylval = (YYSTYPE) processunquoted(yytext, yyleng);
			PDEBUG("Found sub name: \"%s\"\n", yylval);
			BEGIN(INITIAL);
			return TOK_ID;
		}
	{QUOTED_ID}	{
			  /* Ugh, this is a gross hack. I used to use
			   * {ID}+ to match all TOK_IDs, but that would
			   * also match TOK_MODE + TOK_END_OF_RULE
			   * without any spaces in between (because it's
			   * a longer match). So now, when I want to
			   * match any random string, I go into a
			   * seperate state. */
			yylval = (YYSTYPE) processquoted(yytext, yyleng);
			PDEBUG("Found sub name: \"%s\"\n", yylval);
			BEGIN(INITIAL);
			return TOK_ID;
		}

	[^\n]	{
			/* Something we didn't expect */
			yyerror(_("Found unexpected character: '%s'"), yytext);
		}
}

<IP_MODE>{
	{COLON}	{
			PDEBUG("(ip_mode) Found a colon\n");
			return TOK_COLON;
		}
	{NUMBER}	{
			yylval = (YYSTYPE) strtoul(yytext, NULL, 10);
			PDEBUG("(ip_mode) Found a number %d\n", yylval);
			return TOK_NUM;
		}
	{IP}		{
			yylval = (YYSTYPE) strdup(yytext);
			PDEBUG("Found ip %s\n", yylval);
			return TOK_IP;
			}
	{SLASH}	{
			PDEBUG("(ip_mode) Found a slash\n");
			return TOK_SLASH;
			}
	{RANGE}	{
			PDEBUG("(ip_mode) Found a slash\n");
			return TOK_RANGE;
			}
	{WS}	{
			/* Ignoring whitespace */
			PDEBUG("Ending ip mode\n");
			BEGIN(INITIAL);
		}
	{END_OF_RULE}	{ /* Ugh, need this so we don't require a space
			     before the EoL marker */
			BEGIN(INITIAL);
			return TOK_END_OF_RULE;
		}
	[^\n]	{
			  /* Something we didn't expect */
			yylval = (YYSTYPE) strdup(yytext);
			yyerror(_("(ip_mode) Found unexpected character: '%s'"), yylval);
		}
}

<IF_MODE>{
	{WS}+	{ /* Eat whitespace preceding interface */ }

	{IFACE}	{
			yylval = (YYSTYPE) strdup(yytext);
			PDEBUG("Found interface: %s\n", yylval);
			BEGIN(INITIAL);
			return TOK_IFACE;
		}
	[^\n]	{
			/* Something we didn't expect */
			yyerror(_("Unexpected character in interface name: '%s'"), yytext);
		}
}

<FLAGS_MODE>{
	{FLAGOPEN_PAREN}	{
			PDEBUG("FLag (\n");
			return TOK_FLAG_OPENPAREN;
			}
	{FLAGCLOSE_PAREN}	{
			PDEBUG("Flag )\n");
			BEGIN(INITIAL);
			return TOK_FLAG_CLOSEPAREN;
			}

	{WS}+	{ /* Eat whitespace */ }

	{FLAGSEP}	{
			PDEBUG("Flag , \n");
			return TOK_FLAG_SEP;
			}

	{EQUALS}	{
			PDEBUG("Flag = \n");
			return TOK_EQUALS;
			}

	{KEYWORD}	{
			int token = get_keyword_token(yytext);

			/* special cases */
			switch (token) {
			case TOK_FLAG_AUDIT:
			case TOK_FLAG_COMPLAIN:
			case TOK_FLAG_DEBUG:
				/* legit */
				break;
			default:
				/* bad keyword or no token found */
				yyerror(_("Found unknown flag: '%s'"), yytext);
				break;
			}
			return token;
  			}

	[^\n] 		{
			/* Something we didn't expect */
			yyerror(_("Found unexpected character: '%s'"), yytext);
			}
}

<ASSIGN_MODE>{
	{WS}+		{ /* Eat whitespace */ }

	{ID}+		{
			yylval = (YYSTYPE) processunquoted(yytext, yyleng);
			PDEBUG("Found assignment value: \"%s\"\n", yylval);
			return TOK_VALUE;
			}

	{QUOTED_ID}	{
			yylval = (YYSTYPE) processquoted(yytext, yyleng);
			PDEBUG("Found assignment value: \"%s\"\n", yylval);
			return TOK_VALUE;
			}

	\\\n		{ current_lineno++ ; }

	\r?\n		{
			current_lineno++;
			BEGIN(INITIAL);
			}
}

<NETWORK_MODE>{
	{WS}+		{ /* Eat whitespace */ }

	{ID}+		{
			yylval = (YYSTYPE) strdup(yytext);
			return TOK_ID;
			}
	{END_OF_RULE}	{
			BEGIN(INITIAL);
			return TOK_END_OF_RULE;
		}
	[^\n]		{
			  /* Something we didn't expect */
			yylval = (YYSTYPE) strdup(yytext);
			yyerror(_("(network_mode) Found unexpected character: '%s'"), yylval);
			}

	\r?\n		{
			current_lineno++;
			}
}

#.*\n			{ /* Comment - ignore */
			current_lineno++;
			PDEBUG("Line no++: %d\n", current_lineno);
			}

{END_OF_RULE}		{ return TOK_END_OF_RULE; }

{SEPERATOR}		{
			PDEBUG("Matched a seperator\n");
			yylval = (YYSTYPE) yytext;
			BEGIN(SUB_NAME);
			return TOK_SEP;
			}
{EQUALS}		{
			PDEBUG("Matched equals for assignment\n");
			BEGIN(ASSIGN_MODE);
			return TOK_EQUALS;
			}
{ADD_ASSIGN}		{
			PDEBUG("Matched additive value assignment\n");
			BEGIN(ASSIGN_MODE);
			return TOK_ADD_ASSIGN;
			}

{SET_VARIABLE}		{
			yylval = (YYSTYPE) strdup(yytext);
			PDEBUG("Found set variable %s\n", yylval);
			return TOK_SET_VAR;
			}

{BOOL_VARIABLE}		{
			yylval = (YYSTYPE) strdup(yytext);
			PDEBUG("Found boolean variable %s\n", yylval);
			return TOK_BOOL_VAR;
			}

{OPEN_BRACE}		{
			PDEBUG("Open Brace\n");
			return TOK_OPEN;
			}
{CLOSE_BRACE}		{
			PDEBUG("Close Brace\n");
			return TOK_CLOSE;
			}


{IP}			{
			yylval = (YYSTYPE) strdup(yytext);
			PDEBUG("Found ip %s\n", yylval);
			BEGIN(IP_MODE);
			return TOK_IP;
			}

{PATHNAME}		{  
			yylval = (YYSTYPE) processunquoted(yytext, yyleng);
			PDEBUG("Found id: \"%s\"\n", yylval);
			return TOK_ID;
			}

{QPATHNAME}		{  
			yylval = (YYSTYPE) processquoted(yytext, yyleng);
			PDEBUG("Found id: \"%s\"\n", yylval);
			return TOK_ID;
			}

{MODES}			{
			yylval = (YYSTYPE) strdup(yytext);
			PDEBUG("Found modes: %s\n", yylval);
			return TOK_MODE;
			}

{HAT}			{
			BEGIN(SUB_NAME);
			return TOK_HAT;
			}

{KEYWORD}		{
			int token = get_keyword_token(yytext);

			/* special cases */
			switch (token) {
			case -1:
				/* no token found */
				yyerror(_("Found unexpected keyword: '%s'"), yytext);
				break;
			case TOK_VIA:
				BEGIN(IF_MODE);	/* look for an interface name next */
				break;
			case TOK_FLAGS:
				BEGIN(FLAGS_MODE);
				break;
			case TOK_NETWORK:
				BEGIN(NETWORK_MODE);
				break;
			default: /* nothing */
				break;
			}
			return token;
			}

{WS}+			{  /* Ignoring whitespace */ }

\r?\n			{ current_lineno++ ; }

[^\n]			{
			  /* Something we didn't expect */
			yylval = (YYSTYPE) strdup(yytext);
			yyerror(_("Found unexpected character: '%s'"), yylval);
			}

%%
