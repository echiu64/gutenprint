/*
 * "$Id$"
 *
 *   Dump the per-printer options for Grant Taylor's *-omatic database
 *
 *   Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)
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

#include <stdio.h>
#include "print.h"

char *params[] =
{
  "PageSize",
  "Resolution",
  "InkType",
  "MediaType",
  "InputSlot"
};

int nparams = sizeof(params) / sizeof(const char *);

int
main(int argc, char **argv)
{
  int i, j, k;
  for (i = 0; i < known_printers(); i++)
    {
      const printer_t *p = get_printer_by_index(i);
      char **retval;
      int count;
      printf("# Printer model %s, long name `%s'\n", p->driver, p->long_name);
      for (k = 0; k < nparams; k++)
	{
	  retval = (*p->parameters)(p, NULL, params[k], &count);
	  if (count > 0)
	    {
	      for (j = 0; j < count; j++)
		{
		  printf("$stpdata{'%s'}{'%s'}{'%s'} = 1;\n",
			 p->driver, params[k], retval[j]);
		  free(retval[j]);
		}
	      free(retval);
	    }
	}
    }
  return 0;
}


void
Image_init(Image image)
{
}

void
Image_reset(Image image)
{
}

void
Image_transpose(Image image)
{
}

void
Image_hflip(Image image)
{
}

void
Image_vflip(Image image)
{
}

void
Image_crop(Image image, int left, int top, int right, int bottom)
{
}

void
Image_rotate_ccw(Image image)
{
}

void
Image_rotate_cw(Image image)
{
}

void
Image_rotate_180(Image image)
{
}

int
Image_bpp(Image image)
{
  return 0;
}

int
Image_width(Image image)
{
  return 0;
}

int
Image_height(Image image)
{
  return 0;
}

void
Image_get_row(Image image, unsigned char *data, int row)
{
}

void
Image_progress_init(Image image)
{
}

void
Image_note_progress(Image image, double current, double total)
{
}

void
Image_progress_conclude(Image image)
{
}

const char *
Image_get_appname(Image image)
{
  return NULL;
}

	    
