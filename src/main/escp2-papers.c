/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "print-escp2.h"

static const char plain_paper_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"1.20 1.22 1.28 1.34 1.39 1.42 1.45 1.48 "  /* C */
"1.50 1.40 1.30 1.25 1.20 1.10 1.05 1.05 "  /* B */
"1.05 1.05 1.05 1.05 1.05 1.05 1.05 1.05 "  /* M */
"1.05 1.05 1.05 1.10 1.10 1.10 1.10 1.10 "  /* R */
"1.10 1.15 1.30 1.45 1.60 1.75 1.90 2.00 "  /* Y */
"2.10 2.00 1.80 1.70 1.60 1.50 1.40 1.30 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char pgpp_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"1.00 1.00 1.00 1.03 1.05 1.07 1.09 1.11 "  /* B */
"1.13 1.13 1.13 1.13 1.13 1.13 1.13 1.13 "  /* M */
"1.13 1.10 1.05 1.00 1.00 1.00 1.00 1.00 "  /* R */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* Y */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char pgpp_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"1.00 1.00 1.00 1.03 1.05 1.07 1.09 1.11 "  /* B */
"1.13 1.13 1.13 1.13 1.13 1.13 1.13 1.13 "  /* M */
"1.13 1.10 1.05 1.00 1.00 1.00 1.00 1.00 "  /* R */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* Y */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char pgpp_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
"0.00 0.00 0.00 0.00 0.00 0.01 0.02 0.03 "  /* C */
"0.05 0.05 0.05 0.04 0.04 0.03 0.02 0.01 "  /* B */
"0.00 -.03 -.05 -.07 -.09 -.11 -.13 -.14 "  /* M */
"-.15 -.13 -.10 -.06 -.04 -.02 -.01 0.00 "  /* R */
"0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 "  /* Y */
"0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

#define DECLARE_PAPERS(name)				\
const paperlist_t stpi_escp2_##name##_paper_list =	\
{							\
  #name,						\
  sizeof(name##_papers) / sizeof(paper_t),		\
  name##_papers						\
}

#define DECLARE_PAPER_ADJUSTMENTS(name)					  \
const paper_adjustment_list_t stpi_escp2_##name##_paper_adjustment_list = \
{									  \
  #name,								  \
  sizeof(name##_adjustments) / sizeof(paper_adjustment_t),		  \
  name##_adjustments							  \
}

static const paper_adjustment_t standard_adjustments[] =
{
  { "Plain", 0.769, .15, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "PlainFast", 0.769, .15, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Postcard", 0.83, .15, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "GlossyFilm", 1.00, .25, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Transparency", 1.00, .25, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Envelope", 0.769, .15, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "BackFilm", 1.00, .25, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Matte", 0.85, .25, .999, 1.05, 1.0, 0.95, 1, 1.0,
    NULL, NULL, NULL },
  { "Inkjet", 0.85, .15, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Coated", 1.00, .25, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Photo", 1.00, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "GlossyPhoto", 1.10, .25, .999, 1.0, 1.0, 1.0, 1, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "Semigloss", 1.10, .25, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Luster", 1.00, .25, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "GlossyPaper", 1.00, .25, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "Ilford", .85, .2, 1.35, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL  },
  { "ColorLife", 1.00, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Other", 0.769, 0.15, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
};

DECLARE_PAPER_ADJUSTMENTS(standard);

static const paper_adjustment_t photo_adjustments[] =
{
  { "Plain", 0.769, .25, .9, .9, 1.05, 1.15, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "PlainFast", 0.769, .25, .9, .9, 1.05, 1.15, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Postcard", 0.83, .3, .9, .9, 1.0, 1.1, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "GlossyFilm", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Transparency", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Envelope", 0.769, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "BackFilm", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Matte", 0.85, .5, .999, .9, 1.0, 1.1, 1, 1.0,
    NULL, NULL, NULL },
  { "Inkjet", 0.85, .3, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Coated", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Photo", 1.00, .5, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "GlossyPhoto", 1.10, .5, .999, 1.0, 1.03, 1.0, 1, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "Semigloss", 1.10, .5, .999, 1.0, 1.03, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Luster", 1.00, .5, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "GlossyPaper", 1.00, .5, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "Ilford", .85, .4, 1.35, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL  },
  { "ColorLife", 1.00, .4, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Other", 0.769, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
};

DECLARE_PAPER_ADJUSTMENTS(photo);

static const paper_adjustment_t sp780_photo_adjustments[] =
{
  { "Plain", 0.769, .25, .9, .9, 1.05, 1.15, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "PlainFast", 0.769, .25, .9, .9, 1.05, 1.15, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Postcard", 0.83, .3, .9, .9, 1.0, 1.1, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "GlossyFilm", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Transparency", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Envelope", 0.769, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "BackFilm", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Matte", 0.85, .5, .999, .9, 1.0, 1.1, 1, 1.0,
    NULL, NULL, NULL },
  { "Inkjet", 0.85, .3, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Coated", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Photo", 1.00, .5, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "GlossyPhoto", 1.10, .5, .999, 1.0, 1.03, 1.0, 1, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "Semigloss", 1.10, .5, .999, 1.0, 1.03, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Luster", 1.00, .5, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "GlossyPaper", 1.00, .5, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "Ilford", .85, .4, 1.35, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL  },
  { "ColorLife", 1.00, .4, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Other", 0.769, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
};

DECLARE_PAPER_ADJUSTMENTS(sp780_photo);

static const paper_adjustment_t sp960_adjustments[] =
{
  { "Plain", 0.769, .25, .9, .9, 1.05, 1.15, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "PlainFast", 0.769, .25, .9, .9, 1.05, 1.15, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Postcard", 0.83, .3, .9, .9, 1.0, 1.1, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "GlossyFilm", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Transparency", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Envelope", 0.769, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "BackFilm", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Matte", 0.85, .5, .999, .9, 1.0, 1.1, 1, 1.0,
    NULL, NULL, NULL },
  { "Inkjet", 0.85, .3, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Coated", 1.00, .5, .999, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Photo", 1.00, .5, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "GlossyPhoto", 0.9, .4, 1.3, 1.0, 1.0, 1.0, 1, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "Semigloss", 0.9, .1, 1.0, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Luster", 0.825, .1, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "GlossyPaper", 1.00, .5, .999, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "Ilford", .85, .4, 1.35, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL  },
  { "ColorLife", 1.00, .4, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Other", 0.769, .25, .9, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
};

DECLARE_PAPER_ADJUSTMENTS(sp960);

static const paper_adjustment_t ultrachrome_photo_adjustments[] =
{
  { "Plain", 0.769, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "PlainFast", 0.769, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Postcard", 0.83, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "GlossyFilm", 1.00, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Transparency", 1.00, .02, 1.5, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Envelope", 0.769, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "BackFilm", 1.00, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Matte", 0.85, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Inkjet", 0.85, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Coated", 1.00, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Photo", 1.00, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "GlossyPhoto", 0.85, .02, 1.5, 1.0, 1.0, 1.0, 0.9, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "Semigloss", 0.85, .00, 1.5, 1.0, 1.0, 1.0, 0.9, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "Luster", 0.75, .02, 1.5, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "ArchivalMatte", 0.85, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "WaterColorRadiant", 0.765, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "GlossyPaper", 1.00, .02, 1.5, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "Ilford", .85, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL  },
  { "ColorLife", 1.00, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Other", 0.769, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
};

DECLARE_PAPER_ADJUSTMENTS(ultrachrome_photo);

static const paper_adjustment_t ultrachrome_matte_adjustments[] =
{
  { "Plain", 0.769, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "PlainFast", 0.769, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Postcard", 0.83, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "GlossyFilm", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Transparency", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Envelope", 0.769, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "BackFilm", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Matte", 0.85, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Inkjet", 0.85, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
  { "Coated", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Photo", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "GlossyPhoto", 0.85, 0, 1.25, 1.0, 1.0, 1.0, 0.9, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "Semigloss", 0.85, 0, 1.25, 1.0, 1.0, 1.0, 0.9, 1.0,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment },
  { "ArchivalMatte", 0.94, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "WaterColorRadiant", 0.85, .02, 1.5, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Luster", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "GlossyPaper", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1.0, 1.0,
    NULL, NULL, NULL },
  { "Ilford", .85, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL  },
  { "ColorLife", 1.00, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, NULL, NULL },
  { "Other", 0.769, 0, 1.25, 1.0, 1.0, 1.0, 1, 1.0,
    NULL, plain_paper_lum_adjustment, NULL },
};

DECLARE_PAPER_ADJUSTMENTS(ultrachrome_matte);

static const paper_t standard_papers[] =
{
  { "Plain", N_("Plain Paper"), PAPER_PLAIN,
    1, 0, 0x6b, 0x1a, 0x01, NULL },
  { "PlainFast", N_("Plain Paper Fast Load"), PAPER_PLAIN,
    5, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Postcard", N_("Postcard"), PAPER_PLAIN,
    2, 0, 0x00, 0x00, 0x02, NULL },
  { "GlossyFilm", N_("Glossy Film"), PAPER_PHOTO,
    3, 0, 0x6d, 0x00, 0x01, NULL },
  { "Transparency", N_("Transparencies"), PAPER_TRANSPARENCY,
    3, 0, 0x6d, 0x00, 0x02, NULL },
  { "Envelope", N_("Envelopes"), PAPER_PLAIN,
    4, 0, 0x6b, 0x1a, 0x01, NULL },
  { "BackFilm", N_("Back Light Film"), PAPER_TRANSPARENCY,
    6, 0, 0x6d, 0x00, 0x01, NULL },
  { "Matte", N_("Matte Paper"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL },
  { "Inkjet", N_("Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Coated", N_("Photo Quality Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Photo", N_("Photo Paper"), PAPER_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL },
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "Semigloss", N_("Premium Semigloss Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "Luster", N_("Premium Luster Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "GlossyPaper", N_("Photo Quality Glossy Paper"), PAPER_PREMIUM_PHOTO,
    6, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Ilford", N_("Ilford Heavy Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "ColorLife", N_("ColorLife Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL },
  { "Other", N_("Other"), PAPER_PLAIN,
    0, 0, 0x6b, 0x1a, 0x01, NULL },
};

DECLARE_PAPERS(standard);

static const paper_t c80_papers[] =
{
  { "Plain", N_("Plain Paper"), PAPER_PLAIN,
    1, 0, 0x6b, 0x1a, 0x01, NULL },
  { "PlainFast", N_("Plain Paper Fast Load"), PAPER_PLAIN,
    5, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Postcard", N_("Postcard"), PAPER_PLAIN,
    2, 0, 0x00, 0x00, 0x02, NULL },
  { "GlossyFilm", N_("Glossy Film"), PAPER_PHOTO,
    3, 0, 0x6d, 0x00, 0x01, NULL },
  { "Transparency", N_("Transparencies"), PAPER_TRANSPARENCY,
    3, 0, 0x6d, 0x00, 0x02, NULL },
  { "Envelope", N_("Envelopes"), PAPER_PLAIN,
    4, 0, 0x6b, 0x1a, 0x01, NULL },
  { "BackFilm", N_("Back Light Film"), PAPER_TRANSPARENCY,
    6, 0, 0x6d, 0x00, 0x01, NULL },
  { "Matte", N_("Matte Paper"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL },
  { "Inkjet", N_("Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Coated", N_("Photo Quality Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Photo", N_("Photo Paper"), PAPER_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL },
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, "RGB" },
  { "Semigloss", N_("Premium Semigloss Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, "RGB" },
  { "Luster", N_("Premium Luster Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "GlossyPaper", N_("Photo Quality Glossy Paper"), PAPER_PHOTO,
    6, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Ilford", N_("Ilford Heavy Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "ColorLife", N_("ColorLife Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL },
  { "Other", N_("Other"), PAPER_PLAIN,
    0, 0, 0x6b, 0x1a, 0x01, NULL },
};

DECLARE_PAPERS(c80);

static const paper_t ultrachrome_papers[] =
{
  { "Plain", N_("Plain Paper"), PAPER_PLAIN,
    1, 0, 0x6b, 0x1a, 0x01, NULL },
  { "PlainFast", N_("Plain Paper Fast Load"), PAPER_PLAIN,
    5, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Postcard", N_("Postcard"), PAPER_PLAIN,
    2, 0, 0x00, 0x00, 0x02, NULL },
  { "GlossyFilm", N_("Glossy Film"), PAPER_PHOTO,
    3, 0, 0x6d, 0x00, 0x01, NULL },
  { "Transparency", N_("Transparencies"), PAPER_TRANSPARENCY,
    3, 0, 0x6d, 0x00, 0x02, NULL },
  { "Envelope", N_("Envelopes"), PAPER_PLAIN,
    4, 0, 0x6b, 0x1a, 0x01, NULL },
  { "BackFilm", N_("Back Light Film"), PAPER_TRANSPARENCY,
    6, 0, 0x6d, 0x00, 0x01, NULL },
  { "Matte", N_("Matte Paper"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL },
  { "Inkjet", N_("Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Coated", N_("Photo Quality Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Photo", N_("Photo Paper"), PAPER_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL },
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "Semigloss", N_("Premium Semigloss Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "Luster", N_("Premium Luster Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "ArchivalMatte", N_("Archival Matte Paper"), PAPER_PREMIUM_PHOTO,
    7, 0, 0x00, 0x00, 0x02, NULL },
  { "WaterColorRadiant", N_("Watercolor Paper - Radiant White"), PAPER_PREMIUM_PHOTO,
    7, 0, 0x00, 0x00, 0x02, NULL },
  { "GlossyPaper", N_("Photo Quality Glossy Paper"), PAPER_PHOTO,
    6, 0, 0x6b, 0x1a, 0x01, NULL },
  { "Ilford", N_("Ilford Heavy Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL },
  { "ColorLife", N_("ColorLife Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL },
  { "Other", N_("Other"), PAPER_PLAIN,
    0, 0, 0x6b, 0x1a, 0x01, NULL },
};

DECLARE_PAPERS(ultrachrome);
