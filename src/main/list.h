/*
 * "$Id$"
 *
 *   libgimpprint list functions.
 *
 *   Copyright 2002 Roger Leigh (roger@whinlatter.uklinux.net)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gimpprint, etc.
 */

#ifndef GIMP_PRINT_INTERNAL_LIST_H
#define GIMP_PRINT_INTERNAL_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#define COOKIE_LIST    0xbfea218e

struct stp_internal_list_node;

typedef struct stp_internal_list_node
{
  void *data;                          /* data */
  struct stp_internal_list_node *prev; /* previous node */
  struct stp_internal_list_node *next; /* next node */
} stp_internal_list_node_t;


typedef struct stp_internal_list_head
{
  int cookie;			/* Magic cookie */
  int icache;                               /* index no of cached node */
  int length;                               /* number of nodes */
  struct stp_internal_list_node *start;     /* start node */
  struct stp_internal_list_node *end;       /* end node */
  struct stp_internal_list_node *cache;     /* cached node */
  node_freefunc freefunc;	/* callback: free node data */
  node_copyfunc copyfunc;	/* callback: copy node */
  node_namefunc namefunc;	/* callback: get node name */
  node_namefunc long_namefunc;	/* callback: get node long name */
  node_sortfunc sortfunc;	/* callback: compare (sort) nodes */
} stp_internal_list_head_t;


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_LIST_H */
