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


struct stp_internal_list_node;

typedef struct stp_internal_list_node
{
  void *data;                          /* data */
  struct stp_internal_list_node *prev; /* previous node */
  struct stp_internal_list_node *next; /* next node */
} stp_internal_list_node_t;


typedef struct stp_internal_list_head
{
  int icache;                               /* index no of cached node */
  int length;                               /* number of nodes */
  struct stp_internal_list_node *start;     /* start node */
  struct stp_internal_list_node *end;       /* end node */
  struct stp_internal_list_node *cache;     /* cached node */
  void (*freefunc)(stp_list_item_t *item);  /* callback: free node data */
  const char *(*namefunc)(const stp_list_item_t *item);
                                            /* callback: get node name */
  const char *(*long_namefunc)(const stp_list_item_t *item);
                                            /* callback: get node long name */
  int (*sortfunc)(const stp_list_item_t *item1,
		  const stp_list_item_t *item2);
                                            /* callback: sort nodes */
} stp_internal_list_head_t;


