/*
 * "$Id$"
 *
 *   libgimpprint paper functions.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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


#ifndef __GIMP_PRINT_PAPER_H__
#define __GIMP_PRINT_PAPER_H__

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Paper size
 */
typedef enum
{
  PAPERSIZE_ENGLISH_STANDARD,
  PAPERSIZE_METRIC_STANDARD,
  PAPERSIZE_ENGLISH_EXTENDED,
  PAPERSIZE_METRIC_EXTENDED
} stp_papersize_unit_t;

typedef struct
{
  char *name;
  char *text;
  char *comment;
  unsigned width;
  unsigned height;
  unsigned top;
  unsigned left;
  unsigned bottom;
  unsigned right;
  stp_papersize_unit_t paper_unit;
} stp_papersize_t;

/****************************************************************
*                                                               *
* PAPER SIZE MANAGEMENT                                         *
*                                                               *
****************************************************************/

extern int stp_known_papersizes(void);
extern const stp_papersize_t *stp_get_papersize_by_name(const char *name);
extern const stp_papersize_t *stp_get_papersize_by_size(int l, int w);
extern const stp_papersize_t *stp_get_papersize_by_index(int idx);


#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_PAPER_H__ */
/*
 * End of "$Id$".
 */
