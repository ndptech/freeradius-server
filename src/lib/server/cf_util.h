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

/**
 * $Id$
 *
 * @file lib/server/cf_util.h
 * @brief API to create and manipulate internal format configurations.
 *
 * @copyright 2017 The FreeRADIUS server project
 */
RCSIDH(conf_file_h, "$Id$")

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Export the minimum amount of information about these structs
 */
typedef struct cf_item CONF_ITEM;	//!< Generic configuration element, extended to become
					///< a #CONF_PAIR, a #CONF_SECTION or #CONF_DATA.
typedef struct cf_section CONF_SECTION;	//!< #CONF_ITEM used to group multiple #CONF_PAIR and #CONF_SECTION, together.
typedef struct cf_pair CONF_PAIR;	//!< #CONF_ITEM with an attribute, an operator and a value.
typedef struct cf_data CONF_DATA;	//!< #CONF_ITEM used to associate arbitrary data
					///< with a #CONF_PAIR or #CONF_SECTION.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>

#include <freeradius-devel/util/rb.h>
#include <freeradius-devel/util/table.h>
#include <freeradius-devel/util/token.h>
#include <freeradius-devel/util/log.h>

#define FR_TIMEVAL_TO_MS(_x) (((_x)->tv_usec / 1000) + ((_x)->tv_sec * (uint64_t)1000))
#define FR_TIMESPEC_TO_MS(_x) (((_x)->tv_usec / 1000000) + ((_x)->tv_sec * (uint64_t)1000))

/** Auto cast from the input type to CONF_ITEM (which is the base type)
 *
 * Automatically casts:
 * - #CONF_SECTION
 * - #CONF_PAIR
 * - #CONF_DATA
 *
 * To a #CONF_ITEM, whilst performing talloc type checks.
 */
#define CF_TO_ITEM(_cf) \
_Generic((_cf), \
	CONF_SECTION *			: cf_section_to_item((CONF_SECTION const *)_cf), \
	CONF_SECTION const *		: cf_section_to_item((CONF_SECTION const *)_cf), \
	CONF_PAIR *			: cf_pair_to_item((CONF_PAIR const *)_cf), \
	CONF_PAIR const *		: cf_pair_to_item((CONF_PAIR const *)_cf), \
	CONF_DATA *			: cf_data_to_item((CONF_DATA const *)_cf), \
	CONF_DATA const *		: cf_data_to_item((CONF_DATA const *)_cf), \
	default: _cf \
)

typedef int (*cf_walker_t)(void *data, void *ctx);

#define CF_IDENT_ANY ((void *) (-1))

/*
 *	Generic functions that apply to all types of #CONF_ITEM
 */
#define		cf_item_add(_parent, _child) _cf_item_add(CF_TO_ITEM(_parent), CF_TO_ITEM(_child))
void		_cf_item_add(CONF_ITEM *parent, CONF_ITEM *child);

#define		cf_item_insert_after(_parent, _prev, _child) _cf_item_insert_after(CF_TO_ITEM(_parent), CF_TO_ITEM(_prev), CF_TO_ITEM(_child))
void		_cf_item_insert_after(CONF_ITEM *parent, CONF_ITEM *prev, CONF_ITEM *child);

#define		cf_item_remove(_parent, _child) _cf_item_remove(CF_TO_ITEM(_parent), CF_TO_ITEM(_child))
CONF_ITEM	*_cf_item_remove(CONF_ITEM *parent, CONF_ITEM *child);

#define		cf_item_next(_parent, _curr) _cf_item_next(CF_TO_ITEM(_parent), _curr)
CONF_ITEM	*_cf_item_next(CONF_ITEM const *parent, CONF_ITEM const *curr);

#define		cf_item_prev(_parent, _curr) _cf_item_prev(CF_TO_ITEM(_parent), _curr)
CONF_ITEM	*_cf_item_prev(CONF_ITEM const *parent, CONF_ITEM const *prev);

#define		cf_root(_cf) _cf_root(CF_TO_ITEM(_cf))
CONF_SECTION	*_cf_root(CONF_ITEM const *ci);

#define		cf_parent(_cf) _cf_parent(CF_TO_ITEM(_cf))
CONF_ITEM	*_cf_parent(CONF_ITEM const *ci);

#define		cf_lineno(_cf) _cf_lineno(CF_TO_ITEM(_cf))
int		_cf_lineno(CONF_ITEM const *ci);

#define		cf_filename(_cf) _cf_filename(CF_TO_ITEM(_cf))
char const	*_cf_filename(CONF_ITEM const *ci);

bool		cf_item_is_section(CONF_ITEM const *ci);
bool		cf_item_is_pair(CONF_ITEM const *ci);
bool		cf_item_is_data(CONF_ITEM const *ci);

/** @hidecallergraph */
CONF_PAIR	*cf_item_to_pair(CONF_ITEM const *ci);
/** @hidecallergraph */
CONF_SECTION	*cf_item_to_section(CONF_ITEM const *ci);
/** @hidecallergraph */
CONF_DATA	*cf_item_to_data(CONF_ITEM const *ci);

/** @hidecallergraph */
CONF_ITEM	*cf_pair_to_item(CONF_PAIR const *cp);
/** @hidecallergraph */
CONF_ITEM	*cf_section_to_item(CONF_SECTION const *cs);
/** @hidecallergraph */
CONF_ITEM	*cf_data_to_item(CONF_DATA const *cs);

#define		cf_filename_set(_ci, _filename) _cf_filename_set(CF_TO_ITEM(_ci), _filename)
void		_cf_filename_set(CONF_ITEM *cs, char const *filename);

#define		cf_lineno_set(_ci, _lineno) _cf_lineno_set(CF_TO_ITEM(_ci), _lineno)
void		_cf_lineno_set(CONF_ITEM *cs, int lineno);

void		cf_item_free_children(CONF_ITEM *ci);

/*
 *	Section manipulation and searching
 */
#ifndef NDEBUG
#  define	cf_section_alloc(_ctx, _parent, _name1, _name2) \
			_cf_section_alloc(_ctx, _parent, _name1, _name2, __FILE__, __LINE__)
#else
#  define	cf_section_alloc(_ctx, _parent, _name1, _name2) \
			_cf_section_alloc(_ctx, _parent, _name1, _name2, NULL, 0)
#endif
CONF_SECTION	*_cf_section_alloc(TALLOC_CTX *ctx, CONF_SECTION *parent,
				   char const *name1, char const *name2,
				   char const *filename, int lineno);
CONF_SECTION	*cf_section_dup(TALLOC_CTX *ctx, CONF_SECTION *parent, CONF_SECTION const *cs,
				char const *name1, char const *name2, bool copy_meta);

#define cf_section_foreach(_parent, _iter) \
		for (CONF_SECTION *_iter = cf_section_first(_parent); _iter; _iter = cf_section_next(_parent, _iter))

/** @hidecallergraph */
CONF_SECTION	*cf_section_first(CONF_SECTION const *cs);

/** @hidecallergraph */
CONF_SECTION	*cf_section_next(CONF_SECTION const *cs, CONF_SECTION const *curr);

/** @hidecallergraph */
CONF_SECTION	*cf_section_prev(CONF_SECTION const *cs, CONF_SECTION const *curr);

/** @hidecallergraph */
CONF_SECTION	*cf_section_find(CONF_SECTION const *cs, char const *name1, char const *name2);
/** @hidecallergraph */
CONF_SECTION	*cf_section_find_next(CONF_SECTION const *cs, CONF_SECTION const *subcs,
				      char const *name1, char const *name2);

#define		cf_section_find_in_parent(_cf, _name1, _name2) \
		_cf_section_find_in_parent(CF_TO_ITEM(_cf), _name1, _name2)
CONF_SECTION	*_cf_section_find_in_parent(CONF_ITEM const *ci,
					    char const *name1, char const *name2);

#define		cf_section_find_parent(_cf, _name1, _name2) \
		_cf_section_find_parent(CF_TO_ITEM(_cf), _name1, _name2)
CONF_SECTION	*_cf_section_find_parent(CONF_ITEM const *ci,
					 char const *name1, char const *name2);

char const 	*cf_section_value_find(CONF_SECTION const *, char const *attr);

int8_t		cf_section_name_cmp(CONF_SECTION const *cs, char const *name1, char const *name2);
/** @hidecallergraph */
char const	*cf_section_name1(CONF_SECTION const *cs);
/** @hidecallergraph */
char const	*cf_section_name2(CONF_SECTION const *cs);
/** @hidecallergraph */
char const	*cf_section_name(CONF_SECTION const *cs);
char const	*cf_section_argv(CONF_SECTION const *cs, int argc);
fr_token_t	cf_section_name2_quote(CONF_SECTION const *cs);
fr_token_t	cf_section_argv_quote(CONF_SECTION const *cs, int argc);

// only for rewrite_update
void		cf_section_add_name2_quote(CONF_SECTION *cs, fr_token_t token);

#define cf_section_free_children(_x) cf_item_free_children(cf_section_to_item(_x))


/*
 *	Pair manipulation and searching
 */
CONF_PAIR	*cf_pair_alloc(CONF_SECTION *parent, char const *attr, char const *value,
			       fr_token_t op, fr_token_t lhs_type, fr_token_t rhs_type);

CONF_PAIR	*cf_pair_dup(CONF_SECTION *parent, CONF_PAIR *cp, bool copy_meta);

int		cf_pair_replace(CONF_SECTION *cs, CONF_PAIR *cp, char const *value);

void		cf_pair_mark_parsed(CONF_PAIR *cp);

bool		cf_pair_is_parsed(CONF_PAIR *cp);

CONF_PAIR	*cf_pair_first(CONF_SECTION const *cs);

CONF_PAIR	*cf_pair_next(CONF_SECTION const *cs, CONF_PAIR const *curr);

CONF_PAIR	*cf_pair_prev(CONF_SECTION const *cs, CONF_PAIR const *curr);

CONF_PAIR	*cf_pair_find(CONF_SECTION const *cs, char const *name);

CONF_PAIR	*cf_pair_find_next(CONF_SECTION const *cs, CONF_PAIR const *prev, char const *name);

CONF_PAIR	*cf_pair_find_in_parent(CONF_SECTION const *cs, char const *attr);

unsigned int	cf_pair_count_descendents(CONF_SECTION const *cs);

unsigned int	cf_pair_count(CONF_SECTION const *cs, char const *attr);

fr_slen_t	cf_pair_values_concat(fr_sbuff_t *sbuff, CONF_SECTION const *cs, char const *attr, char const *sep);

/** @hidecallergraph */
char const	*cf_pair_attr(CONF_PAIR const *pair);
/** @hidecallergraph */
char const	*cf_pair_value(CONF_PAIR const *pair);
/** @hidecallergraph */
fr_token_t	cf_pair_operator(CONF_PAIR const *pair);

fr_token_t	cf_pair_attr_quote(CONF_PAIR const *pair);
fr_token_t	cf_pair_value_quote(CONF_PAIR const *pair);

/*
 *	Data manipulation and searching
 */
#define		cf_data_find(_cf, _type, _name) _cf_data_find(CF_TO_ITEM(_cf), #_type, _name)
CONF_DATA const	*_cf_data_find(CONF_ITEM const *ci, char const *type, char const *name);

#define		cf_data_find_next(_cf, _prev, _type, _name) _cf_data_find_next(CF_TO_ITEM(_cf), CF_TO_ITEM(_prev), #_type, _name)
CONF_DATA const	*_cf_data_find_next(CONF_ITEM const *ci, CONF_ITEM const *prev, char const *type, char const *name);

#define		cf_data_find_in_parent(_cf, _type, _name) _cf_data_find_in_parent(CF_TO_ITEM(_cf), #_type, _name)
CONF_DATA 	*_cf_data_find_in_parent(CONF_ITEM const *ci, char const *type, char const *name);

void		*cf_data_value(CONF_DATA const *cd);

#define		cf_data_add(_cf, _data, _name, _free) _cf_data_add(CF_TO_ITEM(_cf), (void const *) _data, _name, _free, __FILE__, __LINE__)
CONF_DATA const *_cf_data_add(CONF_ITEM *ci, void const *data, char const *name, bool free, char const *filename, int lineno);

#define		cf_data_add_static(_cf, _data, _type, _name) _cf_data_add_static(CF_TO_ITEM(_cf), _data, #_type, _name, __FILE__, __LINE__)
CONF_DATA const *_cf_data_add_static(CONF_ITEM *ci, void const *data, char const *type, char const *name, char const *filename, int lineno);

/** Remove an item from a parent by type and name
 *
 * @param[in] _cf conf section or pair to remove data from.
 * @param[in] _type of data to remove.
 * @param[in] _name of data to remove.
 */
#define		cf_data_remove(_cf, _type, _name) _cf_data_remove(CF_TO_ITEM(_cf), cf_data_find(_cf, _type, _name))

/** Remove an item from a parent
 *
 * @param[in] _cf conf section or pair to remove data from.
 * @param[in] _cd conf data to remove.
 */
#define		cf_data_remove_by_data(_cf, _cd) _cf_data_remove(CF_TO_ITEM(_cf), _cd)
void		*_cf_data_remove(CONF_ITEM *ci, CONF_DATA const *_cd);

#define		cf_data_walk(_cf, _type, _cb, _ctx) _cf_data_walk(CF_TO_ITEM(_cf), #_type, _cb, _ctx)
int		_cf_data_walk(CONF_ITEM *ci, char const *type, cf_walker_t cb, void *ctx);

/*
 *	Validation
 */
int		cf_pair_in_table(int32_t *out, fr_table_num_sorted_t const *table, size_t table_len, CONF_PAIR *cp);

/*
 *	Error logging
 */

#define		cf_log_err(_cf, _fmt, ...)	_cf_log(L_ERR, CF_TO_ITEM(_cf), __FILE__, __LINE__, _fmt, ## __VA_ARGS__)
#define		cf_log_warn(_cf, _fmt, ...)	_cf_log(L_WARN, CF_TO_ITEM(_cf),  __FILE__, __LINE__, _fmt, ## __VA_ARGS__)
#define		cf_log_info(_cf, _fmt, ...)	_cf_log(L_INFO, CF_TO_ITEM(_cf),  __FILE__, __LINE__, _fmt, ## __VA_ARGS__)
#define		cf_log_debug(_cf, _fmt, ...)	_cf_log(L_DBG, CF_TO_ITEM(_cf),  __FILE__, __LINE__, _fmt, ## __VA_ARGS__)
void		_cf_vlog(fr_log_type_t type, CONF_ITEM const *ci, char const *file, int line, char const *fmt, va_list ap) CC_HINT(format (printf, 5, 0));
void		_cf_log(fr_log_type_t type, CONF_ITEM const *ci, char const *file, int line, char const *fmt, ...) CC_HINT(format (printf, 5, 6));

#define		cf_log_perr(_cf, _fmt, ...) _cf_log_perr(L_ERR, CF_TO_ITEM(_cf),  __FILE__, __LINE__, NULL, _fmt, ## __VA_ARGS__)
#define		cf_log_pwarn(_cf, _fmt, ...) _cf_log_perr(L_WARN, CF_TO_ITEM(_cf),  __FILE__, __LINE__, NULL, _fmt, ## __VA_ARGS__)

void		_cf_vlog_perr(fr_log_type_t type, CONF_ITEM const *ci, char const *file, int line,
			      fr_log_perror_format_t const *f_rules, char const *fmt, va_list ap)
		CC_HINT(format (printf, 6, 0));
void		_cf_log_perr(fr_log_type_t type, CONF_ITEM const *ci, char const *file, int line,
			      fr_log_perror_format_t const *f_rules, char const *fmt, ...)
		CC_HINT(format (printf, 6, 7));

#define		cf_log_debug_prefix(_cf, _fmt, ...) _cf_log_with_filename(L_DBG, CF_TO_ITEM(_cf),  __FILE__, __LINE__, _fmt, ## __VA_ARGS__)
void		_cf_log_with_filename(fr_log_type_t type, CONF_ITEM const *ci, char const *file, int line, char const *fmt, ...) CC_HINT(format (printf, 5, 6));

/** Log an error message against a specified child
 *
 * @param[in] _parent	CONF_SECTION.
 * @param[in] _child	string identifier.
 * @param[in] _fmt	of message.
 * @param[in] ...	arguments.
 */
#define		cf_log_err_by_child(_parent, _child, _fmt, ...) _cf_log_by_child(L_ERR, _parent, _child, __FILE__, __LINE__, _fmt, ## __VA_ARGS__)

/** Log an error message against a specified child, draining the thread local error stack
 *
 * @param[in] _parent	CONF_SECTION.
 * @param[in] _child	string identifier.
 * @param[in] _f_rules	Line prefixes.
 * @param[in] _fmt	of message.
 * @param[in] ...	arguments.
 */
#define		cf_log_perr_by_child(_parent, _child, _f_rules, _fmt, ...) _cf_log_perr_by_child(L_ERR, _parent, _child, __FILE__, __LINE__, _f_rules, _fmt, ## __VA_ARGS__)

/** Log a warning message against a specified child
 *
 * @param[in] _parent	CONF_SECTION.
 * @param[in] _child	string identifier.
 * @param[in] _fmt	of message.
 * @param[in] ...	arguments.
 */
#define		cf_log_warn_by_child(_parent, _child, _fmt, ...) _cf_log_by_child(L_WARN, _parent, _child, __FILE__, __LINE__, _fmt, ## __VA_ARGS__)

/** Log an info message against a specified child
 *
 * @param[in] _parent	CONF_SECTION.
 * @param[in] _child	string identifier.
 * @param[in] _fmt	of message.
 * @param[in] ...	arguments.
 */
#define		cf_log_info_by_child(_parent, _child, _fmt, ...) _cf_log_by_child(L_INFO, _parent, _child, __FILE__, __LINE__, _fmt, ## __VA_ARGS__)

/** Log a debug message against a specified child
 *
 * @param[in] _parent	CONF_SECTION.
 * @param[in] _child	string identifier.
 * @param[in] _fmt	of message.
 * @param[in] ...	arguments.
 */
#define		cf_log_debug_by_child(_parent, _child, _fmt, ...) _cf_log_by_child(L_DBG, _parent, _child, __FILE__, __LINE__, _fmt, ## __VA_ARGS__)
void		_cf_log_by_child(fr_log_type_t type, CONF_SECTION const *parent, char const *child,
				   char const *file, int line, char const *fmt, ...) CC_HINT(format (printf, 6, 7));

void		_cf_log_perr_by_child(fr_log_type_t type, CONF_SECTION const *parent, char const *child,
				      char const *file, int line, fr_log_perror_format_t const *f_rules,
				      char const *fmt, ...) CC_HINT(format (printf, 7, 8));

#define		cf_item_debug(_cf) _cf_item_debug(CF_TO_ITEM(_cf))
void		_cf_item_debug(CONF_ITEM const *ci);

void		cf_pair_debug(CONF_PAIR *cp);
void		cf_section_debug(CONF_SECTION *cs);

#define		cf_canonicalize_error(_ci, _slen, _msg, _str) _cf_canonicalize_error(CF_TO_ITEM(_ci), _slen, _msg, _str)
void		_cf_canonicalize_error(CONF_ITEM *ci, ssize_t slen, char const *msg, char const *str);

#ifdef __cplusplus
}
#endif
