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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#include <limits.h>

typedef struct
{
  const char *name;
  const char *text;
  unsigned width;
  unsigned height;
  unsigned top;
  unsigned left;
  unsigned bottom;
  unsigned right;
  stp_papersize_unit_t paper_unit;
} stp_internal_papersize_t;

/*
 * Sizes are converted to 1/72in, then rounded down so that we don't
 * print off the edge of the paper.
 */
static stp_internal_papersize_t paper_sizes[] =
{
  /* Common imperial page sizes */
  { "Letter",		N_ ("Letter"),
    612,  792, 0, 0, 0, 0, PAPERSIZE_ENGLISH },	/* 8.5in x 11in */
  { "Legal",		N_ ("Legal"),
    612, 1008, 0, 0, 0, 0, PAPERSIZE_ENGLISH },	/* 8.5in x 14in */
  { "Tabloid",		N_ ("Tabloid"),
    792, 1224, 0, 0, 0, 0, PAPERSIZE_ENGLISH },	/*  11in x 17in */
  { "Executive",	N_ ("Executive"),
    522, 756, 0, 0, 0, 0, PAPERSIZE_ENGLISH },	/* 7.25 * 10.5in */
  { "Postcard",		N_ ("Postcard"),
    283,  416, 0, 0, 0, 0, PAPERSIZE_ENGLISH },	/* 100mm x 147mm */
  { "w216h360",		N_ ("3x5"),
    216,  360, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w288h432",		N_ ("4x6"),
    288,  432, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w324h495",		N_ ("Epson 4x6 Photo Paper"),
    324, 495, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w360h504",		N_ ("5x7"),
    360,  504, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w360h576",		N_ ("5x8"),
    360,  576, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w432h576",		N_ ("6x8"),
    432,  576, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "8x10",		N_ ("8x10"),
    576,  720, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "Statement",	N_ ("Manual"),
    396,  612, 0, 0, 0, 0, PAPERSIZE_ENGLISH },	/* 5.5in x 8.5in */
  { "TabloidExtra",	N_ ("12x18"),
    864, 1296, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "SuperB",		N_ ("Super B 13x19"),
    936, 1368, 0, 0, 0, 0, PAPERSIZE_ENGLISH },

  /* Other common photographic paper sizes */
  { "w576h864",		N_ ("8x12"),
    576,  864, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { "w792h1008",	N_ ("11x14"),
    792, 1008, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w1152h1440",	N_ ("16x20"),
    1152, 1440, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w1152h1728",	N_ ("16x24"),
    1152, 1728, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* 20x24 for 35 mm */
  { "w1440h1728",	N_ ("20x24"),
    1440, 1728, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w1440h2160",	N_ ("20x30"),
    1440, 2160, 0, 0, 0, 0, PAPERSIZE_ENGLISH },	/* 24x30 for 35 mm */
  { "w1584h2160",	N_ ("22x30"),
    1440, 2160, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* Common watercolor paper */
  { "w1728h2160",	N_ ("24x30"),
    1728, 2160, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w1728h2592",	N_ ("24x36"),
    1728, 2592, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* Sometimes used for 35 mm */
  { "w2160h2880",	N_ ("30x40"),
    2160, 2880, 0, 0, 0, 0, PAPERSIZE_ENGLISH },

  /* International Paper Sizes (mostly taken from BS4000:1968) */

  /*
   * "A" series: Paper and boards, trimmed sizes
   *
   * "A" sizes are in the ratio 1 : sqrt(2).  A0 has a total area
   * of 1 square metre.  Everything is rounded to the nearest
   * millimetre.  Thus, A0 is 841mm x 1189mm.  Every other A
   * size is obtained by doubling or halving another A size.
   */
  { "w4768h6749",	N_ ("4A"),
    4768, 6749, 0, 0, 0, 0, PAPERSIZE_METRIC },	/* 1682mm x 2378mm */
  { "w3370h4768",	N_ ("2A"),
    3370, 4768, 0, 0, 0, 0, PAPERSIZE_METRIC },	/* 1189mm x 1682mm */
  { "A0",		N_ ("A0"),
    2384, 3370, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  841mm x 1189mm */
  { "A1",		N_ ("A1"),
    1684, 2384, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  594mm x  841mm */
  { "A2",		N_ ("A2"),
    1191, 1684, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  420mm x  594mm */
  { "A3",		N_ ("A3"),
    842, 1191, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  297mm x  420mm */
  { "A4",		N_ ("A4"),
    595,  842, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  210mm x  297mm */
  { "A5",		N_ ("A5"),
    420,  595, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  148mm x  210mm */
  { "A6",		N_ ("A6"),
    297,  420, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  105mm x  148mm */
  { "A7",		N_ ("A7"),
    210,  297, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   74mm x  105mm */
  { "A8",		N_ ("A8"),
    148,  210, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   52mm x   74mm */
  { "A9",		N_ ("A9"),
    105,  148, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   37mm x   52mm */
  { "A10",		N_ ("A10"),
    73,  105, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   26mm x   37mm */

  /*
   * Stock sizes for normal trims.
   * Allowance for trim is 3 millimetres.
   */
  { "w2437h3458",	N_ ("RA0"),
    2437, 3458, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  860mm x 1220mm */
  { "w1729h2437",	N_ ("RA1"),
    1729, 2437, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  610mm x  860mm */
  { "w1218h1729",	N_ ("RA2"),
    1218, 1729, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  430mm x  610mm */
  { "w864h1218",	N_ ("RA3"),
    864, 1218, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  305mm x  430mm */
  { "w609h864",		N_ ("RA4"),
    609,  864, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  215mm x  305mm */

  /*
   * Stock sizes for bled work or extra trims.
   */
  { "w2551h3628",	N_ ("SRA0"),
    2551, 3628, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  900mm x 1280mm */
  { "w1814h2551",	N_ ("SRA1"),
    1814, 2551, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  640mm x  900mm */
  { "w1275h1814",	N_ ("SRA2"),
    1275, 1814, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  450mm x  640mm */
  { "w907h1275",	N_ ("SRA3"),
    907, 1275, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  320mm x  450mm */
  { "w637h907",		N_ ("SRA4"),
    637,  907, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  225mm x  320mm */

  /*
   * "B" series: Posters, wall charts and similar items.
   */
  { "w5669h8016",	N_ ("4B ISO"),
    5669, 8016, 0, 0, 0, 0, PAPERSIZE_METRIC },	/* 2000mm x 2828mm */
  { "w4008h5669",	N_ ("2B ISO"),
    4008, 5669, 0, 0, 0, 0, PAPERSIZE_METRIC },	/* 1414mm x 2000mm */
  { "ISOB0",		N_ ("B0 ISO"),
    2834, 4008, 0, 0, 0, 0, PAPERSIZE_METRIC },	/* 1000mm x 1414mm */
  { "ISOB1",		N_ ("B1 ISO"),
    2004, 2834, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  707mm x 1000mm */
  { "ISOB2",		N_ ("B2 ISO"),
    1417, 2004, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  500mm x  707mm */
  { "ISOB3",		N_ ("B3 ISO"),
    1000, 1417, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  353mm x  500mm */
  { "ISOB4",		N_ ("B4 ISO"),
    708, 1000, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  250mm x  353mm */
  { "ISOB5",		N_ ("B5 ISO"),
    498,  708, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  176mm x  250mm */
  { "ISOB6",		N_ ("B6 ISO"),
    354,  498, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  125mm x  176mm */
  { "ISOB7",		N_ ("B7 ISO"),
    249,  354, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   88mm x  125mm */
  { "ISOB8",		N_ ("B8 ISO"),
    175,  249, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   62mm x   88mm */
  { "ISOB9",		N_ ("B9 ISO"),
    124,  175, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   44mm x   62mm */
  { "ISOB10",		N_ ("B10 ISO"),
    87,  124, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   31mm x   44mm */

  { "B0",		N_ ("B0 JIS"),
    2919, 4127, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B1",		N_ ("B1 JIS"),
    2063, 2919, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B2",		N_ ("B2 JIS"),
    1459, 2063, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B3",		N_ ("B3 JIS"),
    1029, 1459, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B4",		N_ ("B4 JIS"),
    727, 1029, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B5",		N_ ("B5 JIS"),
    518,  727, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B6",		N_ ("B6 JIS"),
    362,  518, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B7",		N_ ("B7 JIS"),
    257,  362, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B8",		N_ ("B8 JIS"),
    180,  257, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B9",		N_ ("B9 JIS"),
    127,  180, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "B10",		N_ ("B10 JIS"),
    90,  127, 0, 0, 0, 0, PAPERSIZE_METRIC },

  /*
   * "C" series: Envelopes or folders suitable for A size stationery.
   */
  { "C0",		N_ ("C0"),
    2599, 3676, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  917mm x 1297mm */
  { "C1",		N_ ("C1"),
    1836, 2599, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  648mm x  917mm */
  { "C2",		N_ ("C2"),
    1298, 1836, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  458mm x  648mm */
  { "C3",		N_ ("C3"),
    918, 1298, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  324mm x  458mm */
  { "C4",		N_ ("C4"),
    649,  918, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  229mm x  324mm */
  { "C5",		N_ ("C5"),
    459,  649, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  162mm x  229mm */
  { "w354h918",		N_ ("B6-C4"),
    354,  918, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  125mm x  324mm */
  { "C6",		N_ ("C6"),
    323,  459, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  114mm x  162mm */
  { "DL",		N_ ("DL"),
    311,  623, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*  110mm x  220mm */
  { "w229h459",		N_ ("C7-6"),
    229,  459, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   81mm x  162mm */
  { "C7",		N_ ("C7"),
    229,  323, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   81mm x  114mm */
  { "C8",		N_ ("C8"),
    161,  229, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   57mm x   81mm */
  { "C9",		N_ ("C9"),
    113,  161, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   40mm x   57mm */
  { "C10",		N_ ("C10"),
    79,  113, 0, 0, 0, 0, PAPERSIZE_METRIC },	/*   28mm x   40mm */

  /*
   * US CAD standard paper sizes
   */
  { "ARCHA",		N_ ("ArchA"),
    648,  864, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "ARCHB",		N_ ("ArchB"),
    864, 1296, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "ARCHC",		N_ ("ArchC"),
    1296, 1728, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "ARCHD",		N_ ("ArchD"),
    1728, 2592, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "ARCHE",		N_ ("ArchE"),
    2592, 3456, 0, 0, 0, 0, PAPERSIZE_ENGLISH },

  /*
   * Foolscap
   */
  { "w612h936",		N_ ("American foolscap"),
    612,  936, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* American foolscap */
  { "w648h936",		N_ ("European foolscap"),
    648,  936, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* European foolscap */

  /*
   * Sizes for book production
   * The BPIF and the Publishers Association jointly recommend ten
   * standard metric sizes for case-bound titles as follows:
   */
  { "w535h697",		N_ ("Crown Quarto"),
    535,  697, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 189mm x 246mm */
  { "w569h731",		N_ ("Large Crown Quarto"),
    569,  731, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 201mm x 258mm */
  { "w620h782",		N_ ("Demy Quarto"),
    620,  782, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 219mm x 276mm */
  { "w671h884",		N_ ("Royal Quarto"),
    671,  884, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 237mm x 312mm */
  /*{ "ISO A4",             595,
    841, PAPERSIZE_METRIC, 0, 0, 0, 0 },    210mm x 297mm */
  { "w348h527",		N_ ("Crown Octavo"),
    348,  527, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 123mm x 186mm */
  { "w365h561",		N_ ("Large Crown Octavo"),
    365,  561, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 129mm x 198mm */
  { "w391h612",		N_ ("Demy Octavo"),
    391,  612, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 138mm x 216mm */
  { "w442h663",		N_ ("Royal Octavo"),
    442,  663, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 156mm x 234mm */
  /*{ N_ ("ISO A5"),             419,
    595, 0, 0, 0, 0, PAPERSIZE_METRIC },    148mm x 210mm */

  /* Paperback sizes in common usage */
  { "w314h504",		N_ ("Small paperback"),
    314, 504, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 111mm x 178mm */
  { "w314h513",		N_ ("Penguin small paperback"),
    314, 513, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 111mm x 181mm */
  { "w365h561",		N_ ("Penguin large paperback"),
    365, 561, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 129mm x 198mm */

  /* Miscellaneous sizes */
  { "w283h420",		N_ ("Hagaki Card"),
    283, 420, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 100 x 148 mm */
  { "w420h567",		N_ ("Oufuku Card"),
    420, 567, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* 148 x 200 mm */
  { "w340h666",		N_ ("Japanese long envelope #3"),
    340, 666, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* Japanese long envelope #3 */
  { "w255h581",		N_ ("Japanese long envelope #4"),
    255, 581, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* Japanese long envelope #4 */
  { "w680h941",		N_ ("Japanese Kaku envelope #4"),
    680, 941, 0, 0, 0, 0, PAPERSIZE_METRIC }, /* Japanese Kaku envelope #4 */
  { "COM10",		N_ ("Commercial 10"),
    297, 684, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* US Commercial 10 env */
  { "w315h414",		N_ ("A2 Invitation"),
    315, 414, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* US A2 invitation */
  { "Monarch",		N_ ("Monarch Envelope"),
    279, 540, 0, 0, 0, 0, PAPERSIZE_ENGLISH }, /* Monarch envelope (3.875 * 7.5) */
  { "Custom",		N_ ("Custom"),
    0, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },

  { "w252",		N_ ("89 mm Roll Paper"),
    252, 0, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "w288",		N_ ("4 Inch Roll Paper"),
    288, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w360",		N_ ("5 Inch Roll Paper"),
    360, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w595",		N_ ("210 mm Roll Paper"),
    595, 0, 0, 0, 0, 0, PAPERSIZE_METRIC },
  { "w936",		N_ ("13 Inch Roll Paper"),
    936, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w1584",		N_ ("22 Inch Roll Paper"),
    1584, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w1728",		N_ ("24 Inch Roll Paper"),
    1728, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w2592",		N_ ("36 Inch Roll Paper"),
    2592, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },
  { "w3168",		N_ ("44 Inch Roll Paper"),
    3168, 0, 0, 0, 0, 0, PAPERSIZE_ENGLISH },

  { "",           "",    0, 0, 0, 0, 0, PAPERSIZE_METRIC }
};

int
stp_known_papersizes(void)
{
  return sizeof(paper_sizes) / sizeof(stp_internal_papersize_t) - 1;
}

const char *
stp_papersize_get_name(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->name;
}

const char *
stp_papersize_get_text(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return _(p->text);
}

unsigned
stp_papersize_get_width(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->width;
}

unsigned
stp_papersize_get_height(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->height;
}

unsigned
stp_papersize_get_top(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->top;
}

unsigned
stp_papersize_get_left(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->left;
}

unsigned
stp_papersize_get_bottom(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->bottom;
}

unsigned
stp_papersize_get_right(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->right;
}

stp_papersize_unit_t
stp_papersize_get_unit(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->paper_unit;
}

#if 1
/*
 * This is, of course, blatantly thread-unsafe.  However, it certainly
 * speeds up genppd by a lot!
 */
const stp_papersize_t
stp_get_papersize_by_name(const char *name)
{
  static int last_used_papersize = 0;
  int base = last_used_papersize;
  int sizes = stp_known_papersizes();
  int i;
  if (!name)
    return NULL;
  for (i = 0; i < sizes; i++)
    {
      int size_to_try = (i + base) % sizes;
      const stp_internal_papersize_t *val = &(paper_sizes[size_to_try]);
      if (!strcmp(val->name, name))
	{
	  last_used_papersize = size_to_try;
	  return (const stp_papersize_t) val;
	}
    }
  return NULL;
}
#else
const stp_papersize_t
stp_get_papersize_by_name(const char *name)
{
  const stp_internal_papersize_t *val = &(paper_sizes[0]);
  if (!name)
    return NULL;
  while (c_strlen(val->name) > 0)
    {
      if (!strcmp(val->name, name))
	return (stp_papersize_t) val;
      val++;
    }
  return NULL;
}
#endif

const stp_papersize_t
stp_get_papersize_by_index(int index)
{
  if (index < 0 || index >= stp_known_papersizes())
    return NULL;
  else
    return (stp_papersize_t) &(paper_sizes[index]);
}

static int
paper_size_mismatch(int l, int w, const stp_internal_papersize_t *val)
{
  int hdiff = abs(l - (int) val->height);
  int vdiff = abs(w - (int) val->width);
  return hdiff + vdiff;
}

const stp_papersize_t
stp_get_papersize_by_size(int l, int w)
{
  int score = INT_MAX;
  const stp_internal_papersize_t *ref = NULL;
  const stp_internal_papersize_t *val = &(paper_sizes[0]);
  int sizes = stp_known_papersizes();
  int i;
  for (i = 0; i < sizes; i++)
    {
      if (val->width == w && val->height == l)
	return (stp_papersize_t) val;
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
  return (stp_papersize_t) ref;
}

void
stp_default_media_size(const stp_printer_t printer,
					/* I - Printer model (not used) */
		       const stp_vars_t v,	/* I */
		       int  *width,		/* O - Width in points */
		       int  *height)	/* O - Height in points */
{
  if (stp_get_page_width(v) > 0 && stp_get_page_height(v) > 0)
    {
      *width = stp_get_page_width(v);
      *height = stp_get_page_height(v);
    }
  else
    {
      const stp_papersize_t papersize =
	stp_get_papersize_by_name(stp_get_media_size(v));
      if (!papersize)
	{
	  *width = 1;
	  *height = 1;
	}
      else
	{
	  *width = stp_papersize_get_width(papersize);
	  *height = stp_papersize_get_height(papersize);
	}
      if (*width == 0)
	*width = 612;
      if (*height == 0)
	*height = 792;
    }
}
