/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

/* #define PRINT_DEBUG */


#include "print.h"
#include <math.h>
#include <limits.h>

#ifndef __GNUC__
#  define inline
#endif /* !__GNUC__ */

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

static vars_t default_vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	1.0,			/* Output brightness */
	100.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1,			/* Y offset (-1 = center) */
	1.0,			/* Screen gamma */
	1.0,			/* Contrast */
	1.0,			/* Cyan */
	1.0,			/* Magenta */
	1.0,			/* Yellow */
	0,			/* Linear */
	1.0,			/* Output saturation */
	1.0,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	0,			/* Unit 0=Inch */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0			/* Page height */
};

static vars_t min_vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	0,			/* Output brightness */
	5.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1,			/* Y offset (-1 = center) */
	0.1,			/* Screen gamma */
	0,			/* Contrast */
	0,			/* Cyan */
	0,			/* Magenta */
	0,			/* Yellow */
	0,			/* Linear */
	0,			/* Output saturation */
	.1,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	0,			/* Unit 0=Inch */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0			/* Page height */
};

static vars_t max_vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	2.0,			/* Output brightness */
	100.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1,			/* Y offset (-1 = center) */
	4.0,			/* Screen gamma */
	4.0,			/* Contrast */
	4.0,			/* Cyan */
	4.0,			/* Magenta */
	4.0,			/* Yellow */
	0,			/* Linear */
	9.0,			/* Output saturation */
	2.0,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	0,			/* Unit 0=Inch */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0			/* Page height */
};

#define ICLAMP(value)				\
do						\
{						\
  if (user->value < min->value)			\
    user->value = min->value;			\
  else if (user->value > max->value)		\
    user->value = max->value;			\
} while (0)

void
merge_printvars(vars_t *user, const vars_t *print)
{
  const vars_t *max = print_maximum_settings();
  const vars_t *min = print_minimum_settings();
  user->cyan = (user->cyan * print->cyan);
  ICLAMP(cyan);
  user->magenta = (user->magenta * print->magenta);
  ICLAMP(magenta);
  user->yellow = (user->yellow * print->yellow);
  ICLAMP(yellow);
  user->contrast = (user->contrast * print->contrast);
  ICLAMP(contrast);
  user->brightness = (user->brightness * print->brightness);
  ICLAMP(brightness);
  user->gamma /= print->gamma;
  ICLAMP(gamma);
  user->saturation *= print->saturation;
  ICLAMP(saturation);
  user->density *= print->density;
  ICLAMP(density);
}

/*
 * 'default_media_size()' - Return the size of a default page size.
 */

/*
 * Sizes are converted to 1/72in, then rounded down so that we don't
 * print off the edge of the paper.
 */
const static papersize_t paper_sizes[] =
{
  /* Common imperial page sizes */
  { "Letter",   612,  792, PAPERSIZE_ENGLISH },	/* 8.5in x 11in */
  { "Legal",    612, 1008, PAPERSIZE_ENGLISH },	/* 8.5in x 14in */
  { "Tabloid",  792, 1224, PAPERSIZE_ENGLISH },	/*  11in x 17in */
  { "Executive", 522, 756, PAPERSIZE_ENGLISH },	/* 7.25 * 10.5in */
  { "Postcard", 283,  416, PAPERSIZE_ENGLISH },	/* 100mm x 147mm */
  { "3x5",	216,  360, PAPERSIZE_ENGLISH },
  { "4x6",      288,  432, PAPERSIZE_ENGLISH },
  { "Epson 4x6 Photo Paper", 324, 495, PAPERSIZE_ENGLISH },
  { "5x7",      360,  504, PAPERSIZE_ENGLISH },
  { "5x8",      360,  576, PAPERSIZE_ENGLISH },
  { "HalfLetter", 396, 612, PAPERSIZE_ENGLISH },
  { "6x8",      432,  576, PAPERSIZE_ENGLISH },
  { "8x10",     576,  720, PAPERSIZE_ENGLISH },
  { "Manual",   396,  612, PAPERSIZE_ENGLISH },	/* 5.5in x 8.5in */
  { "12x18",    864, 1296, PAPERSIZE_ENGLISH },
  { "13x19",    936, 1368, PAPERSIZE_ENGLISH },

  /* Other common photographic paper sizes */
  { "8x12",	576,  864, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { "11x14",    792, 1008, PAPERSIZE_ENGLISH },
  { "16x20",   1152, 1440, PAPERSIZE_ENGLISH },
  { "16x24",   1152, 1728, PAPERSIZE_ENGLISH }, /* 20x24 for 35 mm */
  { "20x24",   1440, 1728, PAPERSIZE_ENGLISH },
  { "20x30",   1440, 2160, PAPERSIZE_ENGLISH },	/* 24x30 for 35 mm */
  { "24x30",   1728, 2160, PAPERSIZE_ENGLISH },
  { "24x36",   1728, 2592, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { "30x40",   2160, 2880, PAPERSIZE_ENGLISH },

  /* International Paper Sizes (mostly taken from BS4000:1968) */

  /*
   * "A" series: Paper and boards, trimmed sizes
   *
   * "A" sizes are in the ratio 1 : sqrt(2).  A0 has a total area
   * of 1 square metre.  Everything is rounded to the nearest
   * millimetre.  Thus, A0 is 841mm x 1189mm.  Every other A
   * size is obtained by doubling or halving another A size.
   */
  { "4A",       4768, 6749, PAPERSIZE_METRIC },	/* 1682mm x 2378mm */
  { "2A",       3370, 4768, PAPERSIZE_METRIC },	/* 1189mm x 1682mm */
  { "A0",       2384, 3370, PAPERSIZE_METRIC },	/*  841mm x 1189mm */
  { "A1",       1684, 2384, PAPERSIZE_METRIC },	/*  594mm x  841mm */
  { "A2",       1191, 1684, PAPERSIZE_METRIC },	/*  420mm x  594mm */
  { "A3",        842, 1191, PAPERSIZE_METRIC },	/*  297mm x  420mm */
  { "A4",        595,  842, PAPERSIZE_METRIC },	/*  210mm x  297mm */
  { "A5",        420,  595, PAPERSIZE_METRIC },	/*  148mm x  210mm */
  { "A6",        297,  420, PAPERSIZE_METRIC },	/*  105mm x  148mm */
  { "A7",        210,  297, PAPERSIZE_METRIC },	/*   74mm x  105mm */
  { "A8",        148,  210, PAPERSIZE_METRIC },	/*   52mm x   74mm */
  { "A9",        105,  148, PAPERSIZE_METRIC },	/*   37mm x   52mm */
  { "A10",        73,  105, PAPERSIZE_METRIC },	/*   26mm x   37mm */

  /*
   * Stock sizes for normal trims.
   * Allowance for trim is 3 millimetres.
   */
  { "RA0",      2437, 3458, PAPERSIZE_METRIC },	/*  860mm x 1220mm */
  { "RA1",      1729, 2437, PAPERSIZE_METRIC },	/*  610mm x  860mm */
  { "RA2",      1218, 1729, PAPERSIZE_METRIC },	/*  430mm x  610mm */
  { "RA3",       864, 1218, PAPERSIZE_METRIC },	/*  305mm x  430mm */
  { "RA4",       609,  864, PAPERSIZE_METRIC },	/*  215mm x  305mm */

  /*
   * Stock sizes for bled work or extra trims.
   */
  { "SRA0",     2551, 3628, PAPERSIZE_METRIC },	/*  900mm x 1280mm */
  { "SRA1",     1814, 2551, PAPERSIZE_METRIC },	/*  640mm x  900mm */
  { "SRA2",     1275, 1814, PAPERSIZE_METRIC },	/*  450mm x  640mm */
  { "SRA3",      907, 1275, PAPERSIZE_METRIC },	/*  320mm x  450mm */
  { "SRA4",      637,  907, PAPERSIZE_METRIC },	/*  225mm x  320mm */

  /*
   * "B" series: Posters, wall charts and similar items.
   */
  { "4B ISO",   5669, 8016, PAPERSIZE_METRIC },	/* 2000mm x 2828mm */
  { "2B ISO",   4008, 5669, PAPERSIZE_METRIC },	/* 1414mm x 2000mm */
  { "B0 ISO",   2834, 4008, PAPERSIZE_METRIC },	/* 1000mm x 1414mm */
  { "B1 ISO",   2004, 2834, PAPERSIZE_METRIC },	/*  707mm x 1000mm */
  { "B2 ISO",   1417, 2004, PAPERSIZE_METRIC },	/*  500mm x  707mm */
  { "B3 ISO",   1000, 1417, PAPERSIZE_METRIC },	/*  353mm x  500mm */
  { "B4 ISO",    708, 1000, PAPERSIZE_METRIC },	/*  250mm x  353mm */
  { "B5 ISO",    498,  708, PAPERSIZE_METRIC },	/*  176mm x  250mm */
  { "B6 ISO",    354,  498, PAPERSIZE_METRIC },	/*  125mm x  176mm */
  { "B7 ISO",    249,  354, PAPERSIZE_METRIC },	/*   88mm x  125mm */
  { "B8 ISO",    175,  249, PAPERSIZE_METRIC },	/*   62mm x   88mm */
  { "B9 ISO",    124,  175, PAPERSIZE_METRIC },	/*   44mm x   62mm */
  { "B10 ISO",    87,  124, PAPERSIZE_METRIC },	/*   31mm x   44mm */
  
  { "B0 JIS",   2919, 4127, PAPERSIZE_METRIC },
  { "B1 JIS",   2063, 2919, PAPERSIZE_METRIC },
  { "B2 JIS",   1459, 2063, PAPERSIZE_METRIC },
  { "B3 JIS",   1029, 1459, PAPERSIZE_METRIC },
  { "B4 JIS",    727, 1029, PAPERSIZE_METRIC },
  { "B5 JIS",    518,  727, PAPERSIZE_METRIC },
  { "B6 JIS",    362,  518, PAPERSIZE_METRIC },
  { "B7 JIS",    257,  362, PAPERSIZE_METRIC },
  { "B8 JIS",    180,  257, PAPERSIZE_METRIC },
  { "B9 JIS",    127,  180, PAPERSIZE_METRIC },
  { "B10 JIS",    90,  127, PAPERSIZE_METRIC },

  /*
   * "C" series: Envelopes or folders suitable for A size stationery.
   */
  { "C0",       2599, 3676, PAPERSIZE_METRIC },	/*  917mm x 1297mm */
  { "C1",       1836, 2599, PAPERSIZE_METRIC },	/*  648mm x  917mm */
  { "C2",       1298, 1836, PAPERSIZE_METRIC },	/*  458mm x  648mm */
  { "C3",        918, 1298, PAPERSIZE_METRIC },	/*  324mm x  458mm */
  { "C4",        649,  918, PAPERSIZE_METRIC },	/*  229mm x  324mm */
  { "C5",        459,  649, PAPERSIZE_METRIC },	/*  162mm x  229mm */
  { "B6-C4",     354,  918, PAPERSIZE_METRIC },	/*  125mm x  324mm */
  { "C6",        323,  459, PAPERSIZE_METRIC },	/*  114mm x  162mm */
  { "DL",        311,  623, PAPERSIZE_METRIC },	/*  110mm x  220mm */
  { "C7-6",      229,  459, PAPERSIZE_METRIC },	/*   81mm x  162mm */
  { "C7",        229,  323, PAPERSIZE_METRIC },	/*   81mm x  114mm */
  { "C8",        161,  229, PAPERSIZE_METRIC },	/*   57mm x   81mm */
  { "C9",        113,  161, PAPERSIZE_METRIC },	/*   40mm x   57mm */
  { "C10",        79,  113, PAPERSIZE_METRIC },	/*   28mm x   40mm */

  /*
   * US CAD standard paper sizes
   */
  { "ArchA",	 648,  864, PAPERSIZE_ENGLISH },
  { "ArchB",	 864, 1296, PAPERSIZE_ENGLISH },
  { "ArchC",	1296, 1728, PAPERSIZE_ENGLISH },
  { "ArchD",	1728, 2592, PAPERSIZE_ENGLISH },
  { "ArchE",	2592, 3456, PAPERSIZE_ENGLISH },

  /*
   * Foolscap
   */
  { "flsa",	 612,  936, PAPERSIZE_ENGLISH }, /* American foolscap */
  { "flse",	 648,  936, PAPERSIZE_ENGLISH }, /* European foolscap */

  /*
   * Sizes for book production
   * The BPIF and the Publishers Association jointly recommend ten
   * standard metric sizes for case-bound titles as follows:
   */
  { "Crown Quarto",       535,  697, PAPERSIZE_METRIC }, /* 189mm x 246mm */
  { "Large Crown Quarto", 569,  731, PAPERSIZE_METRIC }, /* 201mm x 258mm */
  { "Demy Quarto",        620,  782, PAPERSIZE_METRIC }, /* 219mm x 276mm */
  { "Royal Quarto",       671,  884, PAPERSIZE_METRIC }, /* 237mm x 312mm */
/*{ "ISO A4",             595,  841, PAPERSIZE_METRIC },    210mm x 297mm */
  { "Crown Octavo",       348,  527, PAPERSIZE_METRIC }, /* 123mm x 186mm */
  { "Large Crown Octavo", 365,  561, PAPERSIZE_METRIC }, /* 129mm x 198mm */
  { "Demy Octavo",        391,  612, PAPERSIZE_METRIC }, /* 138mm x 216mm */
  { "Royal Octavo",       442,  663, PAPERSIZE_METRIC }, /* 156mm x 234mm */
/*{ "ISO A5",             419,  595, PAPERSIZE_METRIC },    148mm x 210mm */

  /* Paperback sizes in common usage */
  { "Small paperback",         314, 504, PAPERSIZE_METRIC }, /* 111mm x 178mm */
  { "Penguin small paperback", 314, 513, PAPERSIZE_METRIC }, /* 111mm x 181mm */
  { "Penguin large paperback", 365, 561, PAPERSIZE_METRIC }, /* 129mm x 198mm */

  /* Miscellaneous sizes */
  { "Hagaki Card", 283, 420, PAPERSIZE_METRIC }, /* 100 x 148 mm */
  { "Oufuku Card", 420, 567, PAPERSIZE_METRIC }, /* 148 x 200 mm */
  { "Long 3", 340, 666, PAPERSIZE_METRIC }, /* Japanese long envelope #3 */
  { "Long 4", 255, 581, PAPERSIZE_METRIC }, /* Japanese long envelope #4 */
  { "Kaku", 680, 941, PAPERSIZE_METRIC }, /* Japanese Kaku envelope #4 */
  { "Commercial 10", 297, 684, PAPERSIZE_ENGLISH }, /* US Commercial 10 env */
  { "A2 Invitation", 315, 414, PAPERSIZE_ENGLISH }, /* US A2 invitation */
  { "Custom", 0, 0, PAPERSIZE_ENGLISH },

  { "",           0,    0, PAPERSIZE_METRIC }
};

int
known_papersizes(void)
{
  return sizeof(paper_sizes) / sizeof(papersize_t);
}

const papersize_t *
get_papersizes(void)
{
  return paper_sizes;
}

const papersize_t *
get_papersize_by_name(const char *name)
{
  const papersize_t *val = &(paper_sizes[0]);
  while (strlen(val->name) > 0)
    {
      if (!strcasecmp(val->name, name))
	return val;
      val++;
    }
  return NULL;
}

#define IABS(a) ((a) > 0 ? a : -(a))

static int
paper_size_mismatch(int l, int w, const papersize_t *val)
{
  int hdiff = IABS(l - (int) val->height);
  int vdiff = fabs(w - (int) val->width);
  return hdiff + vdiff;
}

const papersize_t *
get_papersize_by_size(int l, int w)
{
  int score = INT_MAX;
  const papersize_t *ref = NULL;
  const papersize_t *val = &(paper_sizes[0]);
  while (strlen(val->name) > 0)
    {
      if (val->width == w && val->height == l)
	return val;
      else
	{
	  int myscore = paper_size_mismatch(l, w, val);
	  if (myscore < score && myscore < 20)
	    {
	      ref = val;
	      score = myscore;
	    }
	}
      val++;
    }
  return ref;
}

void
default_media_size(const printer_t *printer,
					/* I - Printer model (not used) */
		   const vars_t *v,	/* I */
        	   int  *width,		/* O - Width in points */
        	   int  *height)	/* O - Height in points */
{
  if (v->page_width > 0 && v->page_height > 0)
    {
      *width = v->page_width;
      *height = v->page_height;
    }
  else
    {
      const papersize_t *papersize = get_papersize_by_name(v->media_size);
      if (!papersize)
	{
	  *width = 1;
	  *height = 1;
	}
      else
	{
	  *width = papersize->width;
	  *height = papersize->height;
	}
      if (*width == 0)
	*width = 612;
      if (*height == 0)
	*height = 792;
    }
}

/*
 * The list of printers has been moved to printers.c
 */
#include "print-printers.c"

int
known_printers(void)
{
  return printer_count;
}

const printer_t *
get_printers(void)
{
  return printers;
}

const printer_t *
get_printer_by_index(int idx)
{
  return &(printers[idx]);
}

const printer_t *
get_printer_by_long_name(const char *long_name)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->long_name, long_name))
	return val;
      val++;
    }
  return NULL;
}

const printer_t *
get_printer_by_driver(const char *driver)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->driver, driver))
	return val;
      val++;
    }
  return NULL;
}

int
get_printer_index_by_driver(const char *driver)
{
  int idx = 0;
  const printer_t *val = &(printers[0]);
  for (idx = 0; idx < known_printers(); idx++)
    {
      if (!strcmp(val->driver, driver))
	return idx;
      val++;
    }
  return -1;
}

const char *
default_dither_algorithm(void)
{
  return dither_algo_names[0];
}

void
compute_page_parameters(int page_right,	/* I */
			int page_left, /* I */
			int page_top, /* I */
			int page_bottom, /* I */
			double scaling, /* I */
			int image_width, /* I */
			int image_height, /* I */
			Image image, /* IO */
			int *orientation, /* IO */
			int *page_width, /* O */
			int *page_height, /* O */
			int *out_width,	/* O */
			int *out_height, /* O */
			int *left, /* O */
			int *top) /* O */
{
  *page_width  = page_right - page_left;
  *page_height = page_top - page_bottom;

  /* In AUTO orientation, just orient the paper the same way as the image. */

  if (*orientation == ORIENT_AUTO)
    {
      if ((*page_width >= *page_height && image_width >= image_height)
         || (*page_height >= *page_width && image_height >= image_width))
        *orientation = ORIENT_PORTRAIT;
      else
        *orientation = ORIENT_LANDSCAPE;
    }

  if (*orientation == ORIENT_LANDSCAPE)
      Image_rotate_ccw(image);
  else if (*orientation == ORIENT_UPSIDEDOWN)
      Image_rotate_180(image);
  else if (*orientation == ORIENT_SEASCAPE)
      Image_rotate_cw(image);

  image_width  = Image_width(image);
  image_height = Image_height(image);

  /*
   * Calculate width/height...
   */

  if (scaling == 0.0)
    {
      *out_width  = *page_width;
      *out_height = *page_height;
    }
  else if (scaling < 0.0)
    {
      /*
       * Scale to pixels per inch...
       */

      *out_width  = image_width * -72.0 / scaling;
      *out_height = image_height * -72.0 / scaling;
    }
  else
    {
      /*
       * Scale by percent...
       */

      /*
       * Decide which orientation gives the proper fit
       * If we ask for 50%, we do not want to exceed that
       * in either dimension!
       */

      int twidth0 = *page_width * scaling / 100.0;
      int theight0 = twidth0 * image_height / image_width;
      int theight1 = *page_height * scaling / 100.0;
      int twidth1 = theight1 * image_width / image_height;

      *out_width = FMIN(twidth0, twidth1);
      *out_height = FMIN(theight0, theight1);
    }

  if (*out_width == 0)
    *out_width = 1;
  if (*out_height == 0)
    *out_height = 1;

  /*
   * Adjust offsets depending on the page orientation...
   */

  if (*orientation == ORIENT_LANDSCAPE || *orientation == ORIENT_SEASCAPE)
    {
      int x;

      x     = *left;
      *left = *top;
      *top  = x;
    }

  if ((*orientation == ORIENT_UPSIDEDOWN || *orientation == ORIENT_SEASCAPE)
      && *left >= 0)
    {
      *left = *page_width - *left - *out_width;
      if (*left < 0) {
	*left = 0;
      }
    }

  if ((*orientation == ORIENT_UPSIDEDOWN || *orientation == ORIENT_LANDSCAPE)
      && *top >= 0)
    {
      *top = *page_height - *top - *out_height;
      if (*top < 0) {
	*top = 0;
      }
    }

  if (*left < 0)
    *left = (*page_width - *out_width) / 2;

  if (*top < 0)
    *top  = (*page_height - *out_height) / 2;
}

int
verify_printer_params(const printer_t *p, const vars_t *v)
{
  char **vptr;
  int count;
  int i;
  int answer = 1;

  if (strlen(v->media_size) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "PageSize", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_size, vptr[i]))
	      goto good_page_size;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid page size\n", v->media_size);
	}
    good_page_size:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }
  else
    {
      int height, width;
      (*p->limit)(p, v, &width, &height);
#if 0
      fprintf(stderr, "limit %d %d dims %d %d\n", width, height,
	      v->page_width, v->page_height);
#endif
      if (v->page_height <= 0 || v->page_height > height ||
	  v->page_width <= 0 || v->page_width > width)
	{
	  answer = 0;
	  fprintf(stderr, "Image size is not valid\n");
	}
    }

  if (strlen(v->media_type) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "MediaType", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_type, vptr[i]))
	      goto good_media_type;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid media type\n", v->media_type);
	}
    good_media_type:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  if (strlen(v->media_source) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "InputSlot", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_source, vptr[i]))
	      goto good_media_source;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid media source\n", v->media_source);
	}
    good_media_source:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  if (strlen(v->resolution) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "Resolution", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->resolution, vptr[i]))
	      goto good_resolution;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid resolution\n", v->resolution);
	}
    good_resolution:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  if (strlen(v->ink_type) > 0)
    {
      vptr = (*p->parameters)(p, NULL, "InkType", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->ink_type, vptr[i]))
	      goto good_ink_type;
	  answer = 0;
	  fprintf(stderr, "%s is not a valid ink type\n", v->ink_type);
	}
    good_ink_type:
      for (i = 0; i < count; i++)
	free(vptr[i]);
      free(vptr);
    }

  for (i = 0; i < num_dither_algos; i++)
    if (!strcmp(v->dither_algorithm, dither_algo_names[i]))
      return answer;

  fprintf(stderr, "%s is not a valid dither algorithm\n", v->dither_algorithm);
  return 0;
}

const vars_t *
print_default_settings()
{
  return &default_vars;
}

const vars_t *
print_maximum_settings()
{
  return &max_vars;
}

const vars_t *
print_minimum_settings()
{
  return &min_vars;
}

#ifdef QUANTIFY
unsigned quantify_counts[NUM_QUANTIFY_BUCKETS] = {0};
struct timeval quantify_buckets[NUM_QUANTIFY_BUCKETS] = {{0,0}};
int quantify_high_index = 0;
int quantify_first_time = 1;
struct timeval quantify_cur_time;
struct timeval quantify_prev_time;

void print_timers() 
{
    int i;

    printf("Quantify timers:\n");
    for (i = 0; i <= quantify_high_index; i++) {
       if (quantify_counts[i] == 0) continue;
        printf("Bucket %d:\t%ld.%ld s\thit %u times\n", i, quantify_buckets[i].tv_sec, quantify_buckets[i].tv_usec, quantify_counts[i]);
        quantify_buckets[i].tv_sec = 0;
        quantify_buckets[i].tv_usec = 0;
        quantify_counts[i] = 0;
    }
}
#endif
