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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print.h>
#include <gimp-print-internal.h>
#include <gimp-print-intl-internal.h>
#include <math.h>
#include <limits.h>
#include <stdarg.h>

#ifndef __GNUC__
#  define inline
#endif /* !__GNUC__ */

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

static stp_vars_t default_vars =
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

static stp_vars_t min_vars =
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

static stp_vars_t max_vars =
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
stp_merge_printvars(stp_vars_t *user, const stp_vars_t *print)
{
  const stp_vars_t *max = stp_maximum_settings();
  const stp_vars_t *min = stp_minimum_settings();
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
 * 'stp_default_media_size()' - Return the size of a default page size.
 */

/*
 * Sizes are converted to 1/72in, then rounded down so that we don't
 * print off the edge of the paper.
 */
const static stp_papersize_t paper_sizes[] =
{
  /* Common imperial page sizes */
  { N_ ("Letter"),   612,  792, PAPERSIZE_ENGLISH },	/* 8.5in x 11in */
  { N_ ("Legal"),    612, 1008, PAPERSIZE_ENGLISH },	/* 8.5in x 14in */
  { N_ ("Tabloid"),  792, 1224, PAPERSIZE_ENGLISH },	/*  11in x 17in */
  { N_ ("Executive"), 522, 756, PAPERSIZE_ENGLISH },	/* 7.25 * 10.5in */
  { N_ ("Postcard"), 283,  416, PAPERSIZE_ENGLISH },	/* 100mm x 147mm */
  { N_ ("3x5"),	216,  360, PAPERSIZE_ENGLISH },
  { N_ ("4x6"),      288,  432, PAPERSIZE_ENGLISH },
  { N_ ("Epson 4x6 Photo Paper"), 324, 495, PAPERSIZE_ENGLISH },
  { N_ ("5x7"),      360,  504, PAPERSIZE_ENGLISH },
  { N_ ("5x8"),      360,  576, PAPERSIZE_ENGLISH },
  { N_ ("HalfLetter"), 396, 612, PAPERSIZE_ENGLISH },
  { N_ ("6x8"),      432,  576, PAPERSIZE_ENGLISH },
  { N_ ("8x10"),     576,  720, PAPERSIZE_ENGLISH },
  { N_ ("Manual"),   396,  612, PAPERSIZE_ENGLISH },	/* 5.5in x 8.5in */
  { N_ ("12x18"),    864, 1296, PAPERSIZE_ENGLISH },
  { N_ ("13x19"),    936, 1368, PAPERSIZE_ENGLISH },
  { N_ ("Super B"),  936, 1368, PAPERSIZE_ENGLISH },	/* Apparently 13x19. */

  /* Other common photographic paper sizes */
  { N_ ("8x12"),	576,  864, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { N_ ("11x14"),    792, 1008, PAPERSIZE_ENGLISH },
  { N_ ("16x20"),   1152, 1440, PAPERSIZE_ENGLISH },
  { N_ ("16x24"),   1152, 1728, PAPERSIZE_ENGLISH }, /* 20x24 for 35 mm */
  { N_ ("20x24"),   1440, 1728, PAPERSIZE_ENGLISH },
  { N_ ("20x30"),   1440, 2160, PAPERSIZE_ENGLISH },	/* 24x30 for 35 mm */
  { N_ ("24x30"),   1728, 2160, PAPERSIZE_ENGLISH },
  { N_ ("24x36"),   1728, 2592, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { N_ ("30x40"),   2160, 2880, PAPERSIZE_ENGLISH },

  /* International Paper Sizes (mostly taken from BS4000:1968) */

  /*
   * "A" series: Paper and boards, trimmed sizes
   *
   * "A" sizes are in the ratio 1 : sqrt(2).  A0 has a total area
   * of 1 square metre.  Everything is rounded to the nearest
   * millimetre.  Thus, A0 is 841mm x 1189mm.  Every other A
   * size is obtained by doubling or halving another A size.
   */
  { N_ ("4A"),       4768, 6749, PAPERSIZE_METRIC },	/* 1682mm x 2378mm */
  { N_ ("2A"),       3370, 4768, PAPERSIZE_METRIC },	/* 1189mm x 1682mm */
  { N_ ("A0"),       2384, 3370, PAPERSIZE_METRIC },	/*  841mm x 1189mm */
  { N_ ("A1"),       1684, 2384, PAPERSIZE_METRIC },	/*  594mm x  841mm */
  { N_ ("A2"),       1191, 1684, PAPERSIZE_METRIC },	/*  420mm x  594mm */
  { N_ ("A3"),        842, 1191, PAPERSIZE_METRIC },	/*  297mm x  420mm */
  { N_ ("A4"),        595,  842, PAPERSIZE_METRIC },	/*  210mm x  297mm */
  { N_ ("A5"),        420,  595, PAPERSIZE_METRIC },	/*  148mm x  210mm */
  { N_ ("A6"),        297,  420, PAPERSIZE_METRIC },	/*  105mm x  148mm */
  { N_ ("A7"),        210,  297, PAPERSIZE_METRIC },	/*   74mm x  105mm */
  { N_ ("A8"),        148,  210, PAPERSIZE_METRIC },	/*   52mm x   74mm */
  { N_ ("A9"),        105,  148, PAPERSIZE_METRIC },	/*   37mm x   52mm */
  { N_ ("A10"),        73,  105, PAPERSIZE_METRIC },	/*   26mm x   37mm */

  /*
   * Stock sizes for normal trims.
   * Allowance for trim is 3 millimetres.
   */
  { N_ ("RA0"),      2437, 3458, PAPERSIZE_METRIC },	/*  860mm x 1220mm */
  { N_ ("RA1"),      1729, 2437, PAPERSIZE_METRIC },	/*  610mm x  860mm */
  { N_ ("RA2"),      1218, 1729, PAPERSIZE_METRIC },	/*  430mm x  610mm */
  { N_ ("RA3"),       864, 1218, PAPERSIZE_METRIC },	/*  305mm x  430mm */
  { N_ ("RA4"),       609,  864, PAPERSIZE_METRIC },	/*  215mm x  305mm */

  /*
   * Stock sizes for bled work or extra trims.
   */
  { N_ ("SRA0"),     2551, 3628, PAPERSIZE_METRIC },	/*  900mm x 1280mm */
  { N_ ("SRA1"),     1814, 2551, PAPERSIZE_METRIC },	/*  640mm x  900mm */
  { N_ ("SRA2"),     1275, 1814, PAPERSIZE_METRIC },	/*  450mm x  640mm */
  { N_ ("SRA3"),      907, 1275, PAPERSIZE_METRIC },	/*  320mm x  450mm */
  { N_ ("SRA4"),      637,  907, PAPERSIZE_METRIC },	/*  225mm x  320mm */

  /*
   * "B" series: Posters, wall charts and similar items.
   */
  { N_ ("4B ISO"),   5669, 8016, PAPERSIZE_METRIC },	/* 2000mm x 2828mm */
  { N_ ("2B ISO"),   4008, 5669, PAPERSIZE_METRIC },	/* 1414mm x 2000mm */
  { N_ ("B0 ISO"),   2834, 4008, PAPERSIZE_METRIC },	/* 1000mm x 1414mm */
  { N_ ("B1 ISO"),   2004, 2834, PAPERSIZE_METRIC },	/*  707mm x 1000mm */
  { N_ ("B2 ISO"),   1417, 2004, PAPERSIZE_METRIC },	/*  500mm x  707mm */
  { N_ ("B3 ISO"),   1000, 1417, PAPERSIZE_METRIC },	/*  353mm x  500mm */
  { N_ ("B4 ISO"),    708, 1000, PAPERSIZE_METRIC },	/*  250mm x  353mm */
  { N_ ("B5 ISO"),    498,  708, PAPERSIZE_METRIC },	/*  176mm x  250mm */
  { N_ ("B6 ISO"),    354,  498, PAPERSIZE_METRIC },	/*  125mm x  176mm */
  { N_ ("B7 ISO"),    249,  354, PAPERSIZE_METRIC },	/*   88mm x  125mm */
  { N_ ("B8 ISO"),    175,  249, PAPERSIZE_METRIC },	/*   62mm x   88mm */
  { N_ ("B9 ISO"),    124,  175, PAPERSIZE_METRIC },	/*   44mm x   62mm */
  { N_ ("B10 ISO"),    87,  124, PAPERSIZE_METRIC },	/*   31mm x   44mm */
  
  { N_ ("B0 JIS"),   2919, 4127, PAPERSIZE_METRIC },
  { N_ ("B1 JIS"),   2063, 2919, PAPERSIZE_METRIC },
  { N_ ("B2 JIS"),   1459, 2063, PAPERSIZE_METRIC },
  { N_ ("B3 JIS"),   1029, 1459, PAPERSIZE_METRIC },
  { N_ ("B4 JIS"),    727, 1029, PAPERSIZE_METRIC },
  { N_ ("B5 JIS"),    518,  727, PAPERSIZE_METRIC },
  { N_ ("B6 JIS"),    362,  518, PAPERSIZE_METRIC },
  { N_ ("B7 JIS"),    257,  362, PAPERSIZE_METRIC },
  { N_ ("B8 JIS"),    180,  257, PAPERSIZE_METRIC },
  { N_ ("B9 JIS"),    127,  180, PAPERSIZE_METRIC },
  { N_ ("B10 JIS"),    90,  127, PAPERSIZE_METRIC },

  /*
   * "C" series: Envelopes or folders suitable for A size stationery.
   */
  { N_ ("C0"),       2599, 3676, PAPERSIZE_METRIC },	/*  917mm x 1297mm */
  { N_ ("C1"),       1836, 2599, PAPERSIZE_METRIC },	/*  648mm x  917mm */
  { N_ ("C2"),       1298, 1836, PAPERSIZE_METRIC },	/*  458mm x  648mm */
  { N_ ("C3"),        918, 1298, PAPERSIZE_METRIC },	/*  324mm x  458mm */
  { N_ ("C4"),        649,  918, PAPERSIZE_METRIC },	/*  229mm x  324mm */
  { N_ ("C5"),        459,  649, PAPERSIZE_METRIC },	/*  162mm x  229mm */
  { N_ ("B6-C4"),     354,  918, PAPERSIZE_METRIC },	/*  125mm x  324mm */
  { N_ ("C6"),        323,  459, PAPERSIZE_METRIC },	/*  114mm x  162mm */
  { N_ ("DL"),        311,  623, PAPERSIZE_METRIC },	/*  110mm x  220mm */
  { N_ ("C7-6"),      229,  459, PAPERSIZE_METRIC },	/*   81mm x  162mm */
  { N_ ("C7"),        229,  323, PAPERSIZE_METRIC },	/*   81mm x  114mm */
  { N_ ("C8"),        161,  229, PAPERSIZE_METRIC },	/*   57mm x   81mm */
  { N_ ("C9"),        113,  161, PAPERSIZE_METRIC },	/*   40mm x   57mm */
  { N_ ("C10"),        79,  113, PAPERSIZE_METRIC },	/*   28mm x   40mm */

  /*
   * US CAD standard paper sizes
   */
  { N_ ("ArchA"),	 648,  864, PAPERSIZE_ENGLISH },
  { N_ ("ArchB"),	 864, 1296, PAPERSIZE_ENGLISH },
  { N_ ("ArchC"),	1296, 1728, PAPERSIZE_ENGLISH },
  { N_ ("ArchD"),	1728, 2592, PAPERSIZE_ENGLISH },
  { N_ ("ArchE"),	2592, 3456, PAPERSIZE_ENGLISH },

  /*
   * Foolscap
   */
  { N_ ("flsa"),	 612,  936, PAPERSIZE_ENGLISH }, /* American foolscap */
  { N_ ("flse"),	 648,  936, PAPERSIZE_ENGLISH }, /* European foolscap */

  /*
   * Sizes for book production
   * The BPIF and the Publishers Association jointly recommend ten
   * standard metric sizes for case-bound titles as follows:
   */
  { N_ ("Crown Quarto"),       535,  697, PAPERSIZE_METRIC }, /* 189mm x 246mm */
  { N_ ("Large Crown Quarto"), 569,  731, PAPERSIZE_METRIC }, /* 201mm x 258mm */
  { N_ ("Demy Quarto"),        620,  782, PAPERSIZE_METRIC }, /* 219mm x 276mm */
  { N_ ("Royal Quarto"),       671,  884, PAPERSIZE_METRIC }, /* 237mm x 312mm */
/*{ "ISO A4",             595,  841, PAPERSIZE_METRIC },    210mm x 297mm */
  { N_ ("Crown Octavo"),       348,  527, PAPERSIZE_METRIC }, /* 123mm x 186mm */
  { N_ ("Large Crown Octavo"), 365,  561, PAPERSIZE_METRIC }, /* 129mm x 198mm */
  { N_ ("Demy Octavo"),        391,  612, PAPERSIZE_METRIC }, /* 138mm x 216mm */
  { N_ ("Royal Octavo"),       442,  663, PAPERSIZE_METRIC }, /* 156mm x 234mm */
/*{ N_ ("ISO A5"),             419,  595, PAPERSIZE_METRIC },    148mm x 210mm */

  /* Paperback sizes in common usage */
  { N_ ("Small paperback"),         314, 504, PAPERSIZE_METRIC }, /* 111mm x 178mm */
  { N_ ("Penguin small paperback"), 314, 513, PAPERSIZE_METRIC }, /* 111mm x 181mm */
  { N_ ("Penguin large paperback"), 365, 561, PAPERSIZE_METRIC }, /* 129mm x 198mm */

  /* Miscellaneous sizes */
  { N_ ("Hagaki Card"), 283, 420, PAPERSIZE_METRIC }, /* 100 x 148 mm */
  { N_ ("Oufuku Card"), 420, 567, PAPERSIZE_METRIC }, /* 148 x 200 mm */
  { N_ ("Long 3"), 340, 666, PAPERSIZE_METRIC }, /* Japanese long envelope #3 */
  { N_ ("Long 4"), 255, 581, PAPERSIZE_METRIC }, /* Japanese long envelope #4 */
  { N_ ("Kaku"), 680, 941, PAPERSIZE_METRIC }, /* Japanese Kaku envelope #4 */
  { N_ ("Commercial 10"), 297, 684, PAPERSIZE_ENGLISH }, /* US Commercial 10 env */
  { N_ ("A2 Invitation"), 315, 414, PAPERSIZE_ENGLISH }, /* US A2 invitation */
  { N_ ("Custom"), 0, 0, PAPERSIZE_ENGLISH },

  { "",           0,    0, PAPERSIZE_METRIC }
};

int
stp_known_papersizes(void)
{
  return sizeof(paper_sizes) / sizeof(stp_papersize_t);
}

const stp_papersize_t *
stp_get_papersizes(void)
{
  return paper_sizes;
}

const stp_papersize_t *
stp_get_papersize_by_name(const char *name)
{
  const stp_papersize_t *val = &(paper_sizes[0]);
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
paper_size_mismatch(int l, int w, const stp_papersize_t *val)
{
  int hdiff = IABS(l - (int) val->height);
  int vdiff = fabs(w - (int) val->width);
  return hdiff + vdiff;
}

const stp_papersize_t *
stp_get_papersize_by_size(int l, int w)
{
  int score = INT_MAX;
  const stp_papersize_t *ref = NULL;
  const stp_papersize_t *val = &(paper_sizes[0]);
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
stp_default_media_size(const stp_printer_t *printer,
					/* I - Printer model (not used) */
		   const stp_vars_t *v,	/* I */
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
      const stp_papersize_t *papersize = stp_get_papersize_by_name(v->media_size);
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
stp_known_printers(void)
{
  return printer_count;
}

const stp_printer_t *
stp_get_printers(void)
{
  return printers;
}

const stp_printer_t *
stp_get_printer_by_index(int idx)
{
  return &(printers[idx]);
}

const stp_printer_t *
stp_get_printer_by_long_name(const char *long_name)
{
  const stp_printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < stp_known_printers(); i++)
    {
      if (!strcmp(val->long_name, long_name))
	return val;
      val++;
    }
  return NULL;
}

const stp_printer_t *
stp_get_printer_by_driver(const char *driver)
{
  const stp_printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < stp_known_printers(); i++)
    {
      if (!strcmp(val->driver, driver))
	return val;
      val++;
    }
  return NULL;
}

int
stp_get_printer_index_by_driver(const char *driver)
{
  int idx = 0;
  const stp_printer_t *val = &(printers[0]);
  for (idx = 0; idx < stp_known_printers(); idx++)
    {
      if (!strcmp(val->driver, driver))
	return idx;
      val++;
    }
  return -1;
}

const char *
stp_default_dither_algorithm(void)
{
  return stp_dither_algorithm_name(0);
}

void
stp_compute_page_parameters(int page_right,	/* I */
			    int page_left, /* I */
			    int page_top, /* I */
			    int page_bottom, /* I */
			    double scaling, /* I */
			    int image_width, /* I */
			    int image_height, /* I */
			    stp_image_t *image, /* IO */
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
      image->rotate_ccw(image);
  else if (*orientation == ORIENT_UPSIDEDOWN)
      image->rotate_180(image);
  else if (*orientation == ORIENT_SEASCAPE)
      image->rotate_cw(image);

  image_width  = image->width(image);
  image_height = image->height(image);

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
stp_verify_printer_params(const stp_printer_t *p, const stp_vars_t *v)
{
  char **vptr;
  int count;
  int i;
  int answer = 1;

  if (strlen(v->media_size) > 0)
    {
      vptr = (*p->printfuncs->parameters)(p, NULL, "PageSize", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_size, vptr[i]))
	      goto good_page_size;
	  answer = 0;
	  stp_eprintf(v, "%s is not a valid page size\n", v->media_size);
	good_page_size:
	  for (i = 0; i < count; i++)
	    free(vptr[i]);
	}
      if (vptr)
	free(vptr);
    }
  else
    {
      int height, width;
      (*p->printfuncs->limit)(p, v, &width, &height);
#if 0
      stp_eprintf(v, "limit %d %d dims %d %d\n",
		  width, height, v->page_width, v->page_height);
#endif
      if (v->page_height <= 0 || v->page_height > height ||
	  v->page_width <= 0 || v->page_width > width)
	{
	  answer = 0;
	  stp_eprintf(v, "Image size is not valid\n");
	}
    }

  if (strlen(v->media_type) > 0)
    {
      vptr = (*p->printfuncs->parameters)(p, NULL, "MediaType", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_type, vptr[i]))
	      goto good_media_type;
	  answer = 0;
	  stp_eprintf(v, "%s is not a valid media type\n", v->media_type);
	good_media_type:
	  for (i = 0; i < count; i++)
	    free(vptr[i]);
	}
      if (vptr)
	free(vptr);
    }

  if (strlen(v->media_source) > 0)
    {
      vptr = (*p->printfuncs->parameters)(p, NULL, "InputSlot", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->media_source, vptr[i]))
	      goto good_media_source;
	  answer = 0;
	  stp_eprintf(v, "%s is not a valid media source\n", v->media_source);
	good_media_source:
	  for (i = 0; i < count; i++)
	    free(vptr[i]);
	}
      if (vptr)
	free(vptr);
    }

  if (strlen(v->resolution) > 0)
    {
      vptr = (*p->printfuncs->parameters)(p, NULL, "Resolution", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->resolution, vptr[i]))
	      goto good_resolution;
	  answer = 0;
	  stp_eprintf(v, "%s is not a valid resolution\n", v->resolution);
	good_resolution:
	  for (i = 0; i < count; i++)
	    free(vptr[i]);
	}
      if (vptr)
	free(vptr);
    }

  if (strlen(v->ink_type) > 0)
    {
      vptr = (*p->printfuncs->parameters)(p, NULL, "InkType", &count);
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(v->ink_type, vptr[i]))
	      goto good_ink_type;
	  answer = 0;
	  stp_eprintf(v, "%s is not a valid ink type\n", v->ink_type);
	good_ink_type:
	  for (i = 0; i < count; i++)
	    free(vptr[i]);
	}
      if (vptr)
	free(vptr);
    }

  for (i = 0; i < stp_dither_algorithm_count(); i++)
    if (!strcmp(v->dither_algorithm, stp_dither_algorithm_name(i)))
      return answer;

  stp_eprintf(v, "%s is not a valid dither algorithm\n", v->dither_algorithm);
  return 0;
}

const stp_vars_t *
stp_default_settings()
{
  return &default_vars;
}

const stp_vars_t *
stp_maximum_settings()
{
  return &max_vars;
}

const stp_vars_t *
stp_minimum_settings()
{
  return &min_vars;
}

extern int vasprintf (char **result, const char *format, va_list args);

void
stp_zprintf(const stp_vars_t *v, const char *format, ...)
{
  va_list args;
  int bytes;
  char *result;
  va_start(args, format);
  bytes = vasprintf(&result, format, args);
  va_end(args);
  (v->outfunc)((void *)(v->outdata), result, bytes);
  free(result);
}

void
stp_zfwrite(const char *buf, size_t bytes, size_t nitems, const stp_vars_t *v)
{
  (v->outfunc)((void *)(v->outdata), buf, bytes * nitems);
}

void
stp_putc(int ch, const stp_vars_t *v)
{
  char a = (char) ch;
  (v->outfunc)((void *)(v->outdata), &a, 1);
}

void
stp_puts(const char *s, const stp_vars_t *v)
{
  (v->outfunc)((void *)(v->outdata), s, strlen(s));
}

void
stp_eprintf(const stp_vars_t *v, const char *format, ...)
{
  va_list args;
  int bytes;
  char *result;
  va_start(args, format);
  bytes = vasprintf(&result, format, args);
  va_end(args);
  (v->errfunc)((void *)(v->errdata), result, bytes);
  free(result);
}
  


#ifdef QUANTIFY
unsigned quantify_counts[NUM_QUANTIFY_BUCKETS] = {0};
struct timeval quantify_buckets[NUM_QUANTIFY_BUCKETS] = {{0,0}};
int quantify_high_index = 0;
int quantify_first_time = 1;
struct timeval quantify_cur_time;
struct timeval quantify_prev_time;

void print_timers(const stp_vars_t *v)
{
  int i;

  stp_eprintf(v, "%s", "Quantify timers:\n");
  for (i = 0; i <= quantify_high_index; i++)
    {
      if (quantify_counts[i] > 0)
	{
	  stp_eprintf(v,
		      "Bucket %d:\t%ld.%ld s\thit %u times\n", i,
		      quantify_buckets[i].tv_sec, quantify_buckets[i].tv_usec,
		      quantify_counts[i]);
	  quantify_buckets[i].tv_sec = 0;
	  quantify_buckets[i].tv_usec = 0;
	  quantify_counts[i] = 0;
	}
    }
}
#endif
