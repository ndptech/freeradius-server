#pragma once
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/** Tokenisation code and constants
 *
 * This is mostly for the attribute filter and user files.
 *
 * @file src/lib/util/token.h
 *
 * @copyright 2001,2006 The FreeRADIUS server project
 */
RCSIDH(token_h, "$Id$")

#ifdef __cplusplus
extern "C" {
#endif

#include <freeradius-devel/build.h>
#include <freeradius-devel/missing.h>
#include <freeradius-devel/util/table.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum fr_token {
	T_INVALID = 0,			/* invalid token */
	T_EOL,				/* end of line */
	T_LCBRACE,			/* { */
	T_RCBRACE,			/* } */
	T_LBRACE,			/* ( */
	T_RBRACE,			/* ) */
	T_COMMA,			/* , */
	T_SEMICOLON,			/* ; */

	/*
	 *	Binary operations
	 */
	T_ADD,				/* + */
	T_SUB,				/* - */
	T_MUL,				/* * */
	T_DIV,				/* / */
	T_AND,				/* & */
	T_OR,				/* | */
	T_NOT,				/* ! */
	T_XOR,				/* ^ */
	T_COMPLEMENT,	       		/* ~ */
	T_MOD,		       		/* % */

	T_RSHIFT,			/* >> */
	T_LSHIFT,			/* << */

	/*
	 *	Assignment operators associated with binary
	 *	operations.
	 */
	T_OP_ADD_EQ,			/* += */
	T_OP_SUB_EQ,			/* -= */
	T_OP_MUL_EQ,			/* *= */
	T_OP_DIV_EQ,			/* /= */
	T_OP_OR_EQ,			/* |= */
	T_OP_AND_EQ,			/* &= */

	T_OP_RSHIFT_EQ,			/* >>= */
	T_OP_LSHIFT_EQ,			/* <<= */

	/*
	 *	Assignment operators associated with
	 *	other operations.
	 */
	T_OP_EQ,			/* = */
	T_OP_SET,			/* := */
	T_OP_PREPEND,			/* ^= */
#define T_OP_XOR_EQ T_OP_PREPEND

	/*
	 *	Logical / short-circuit operators.
	 */
	T_LAND,				/* && */
	T_LOR,				/* || */

	/*
	 *	Comparison operators.
	 */
	T_OP_NE,			/* != */
	T_OP_GE,			/* >= */
	T_OP_GT,			/* >  */
	T_OP_LE,			/* <= */
	T_OP_LT,			/* < */
	T_OP_REG_EQ,			/* =~ */
	T_OP_REG_NE,			/* !~ */
	T_OP_CMP_TRUE,			/* =* */
	T_OP_CMP_FALSE,			/* !* */
	T_OP_CMP_EQ,			/* == */
	T_OP_CMP_EQ_TYPE,      		/* === */
	T_OP_CMP_NE_TYPE,      		/* !== */

	/*
	 *	Unary calc operator.
	 */
	T_OP_INCRM,			/* ++ */

	/*
	 *	T_HASH MUST be after all of various assignment
	 *	operators.  See fr_token_quote[].
	 */
	T_HASH,				/* # */
	T_BARE_WORD,			/* bare word */
	T_DOUBLE_QUOTED_STRING,		/* "foo" */
	T_SINGLE_QUOTED_STRING,		/* 'foo' */
	T_BACK_QUOTED_STRING,		/* `foo` */
	T_SOLIDUS_QUOTED_STRING,	/* /foo/ */
} fr_token_t;
/*
 *	This must be manually updated, and is never part of the ENUM.
 */
#define T_TOKEN_LAST (T_SOLIDUS_QUOTED_STRING + 1)

#define T_EQSTART	T_OP_ADD_EQ
#define	T_EQEND		(T_HASH)

/** Macro to use as dflt
 *
 */
#define FR_TABLE_NOT_FOUND	INT32_MIN

extern fr_table_num_ordered_t const fr_tokens_table[];
extern size_t fr_tokens_table_len;
extern fr_table_num_sorted_t const fr_token_quotes_table[];
extern size_t fr_token_quotes_table_len;
extern const char *fr_tokens[T_TOKEN_LAST];
extern const char fr_token_quote[T_TOKEN_LAST];
extern const bool fr_assignment_op[T_TOKEN_LAST];
extern const bool fr_comparison_op[T_TOKEN_LAST];
extern const bool fr_binary_op[T_TOKEN_LAST];
extern const bool fr_str_tok[T_TOKEN_LAST];
extern const bool fr_list_assignment_op[T_TOKEN_LAST];

int		getword (char const **ptr, char *buf, int buflen, bool unescape);
fr_token_t	gettoken(char const **ptr, char *buf, int buflen, bool unescape);
fr_token_t	getop(char const **ptr);
fr_token_t	getstring(char const **ptr, char *buf, int buflen, bool unescape);
char const	*fr_token_name(int);

#ifdef __cplusplus
}
#endif
