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

static const char standard_sat_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 0.95 0.90 "  /* M */
/* M */  "0.90 0.90 0.90 0.90 0.90 0.90 0.90 0.90 "  /* R */
/* R */  "0.90 0.95 0.95 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char standard_lum_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.50 0.51 0.53 0.57 0.61 0.71 0.81 0.74 "  /* B */
/* B */  "0.68 0.70 0.73 0.76 0.80 0.87 0.92 0.95 "  /* M */
/* M */  "0.97 0.97 0.96 0.95 0.95 0.95 0.95 0.95 "  /* R */
/* R */  "0.95 0.96 0.97 0.98 0.99 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 0.99 0.92 0.80 0.68 0.61 "  /* G */
/* G */  "0.54 0.54 0.54 0.54 0.53 0.53 0.52 0.50 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char standard_hue_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.06 0.10 0.10 0.06 -.01 -.09 -.17 "  /* B */
/* B */  "-.25 -.33 -.38 -.38 -.36 -.34 -.34 -.34 "  /* M */
/* M */  "-.34 -.34 -.36 -.40 -.50 -.40 -.30 -.20 "  /* R */
/* R */  "-.12 -.07 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 0.00 -.05 -.10 -.15 -.22 -.24 "  /* G */
/* G */  "-.26 -.30 -.33 -.28 -.25 -.20 -.13 -.06 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char photo2_sat_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
/* R */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char photo2_lum_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.52 0.55 0.62 0.68 0.73 0.78 0.78 0.75 "  /* B */
/* B */  "0.72 0.70 0.70 0.72 0.78 0.83 0.85 0.88 "  /* M */
/* M */  "0.98 0.97 0.96 0.95 0.95 0.95 0.95 0.95 "  /* R */
/* R */  "0.95 0.96 0.97 0.98 0.99 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 0.99 0.92 0.80 0.68 0.61 "  /* G */
/* G */  "0.54 0.54 0.54 0.54 0.53 0.53 0.52 0.50 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char photo2_hue_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.06 0.10 0.10 0.06 -.01 -.09 -.17 "  /* B */
/* B */  "-.25 -.33 -.38 -.38 -.36 -.34 -.34 -.34 "  /* M */
/* M */  "-.34 -.34 -.36 -.40 -.50 -.40 -.30 -.20 "  /* R */
/* R */  "-.12 -.07 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 0.00 -.05 -.10 -.15 -.22 -.24 "  /* G */
/* G */  "-.26 -.30 -.33 -.28 -.25 -.20 -.13 -.06 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char photo2_luster_lum_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.60 0.62 0.68 0.72 0.76 0.78 0.78 0.75 "  /* B */
/* B */  "0.72 0.70 0.70 0.72 0.78 0.83 0.85 0.88 "  /* M */
/* M */  "0.98 0.97 0.96 0.95 0.95 0.95 0.95 0.95 "  /* R */
/* R */  "0.95 0.96 0.97 0.98 0.99 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 0.95 0.85 0.78 0.74 0.72 0.70 0.68 "  /* G */
/* G */  "0.63 0.63 0.62 0.62 0.62 0.62 0.62 0.60 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


static const char sp960_sat_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.10 1.15 1.10 1.00 1.00 "  /* M */
/* M */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
/* R */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.05 1.10 1.20 1.10 1.05 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char sp960_lum_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.45 0.48 0.52 0.56 0.61 0.73 0.82 0.74 "  /* B */
/* B */  "0.68 0.68 0.68 0.68 0.72 0.75 0.85 0.95 "  /* M */
/* M */  "1.05 1.05 1.05 1.05 1.05 1.05 1.05 1.05 "  /* R */
/* R */  "1.05 1.05 1.05 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 0.95 0.90 0.85 0.80 0.72 0.61 0.54 "  /* G */
/* G */  "0.48 0.46 0.46 0.46 0.46 0.46 0.46 0.46 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char sp960_hue_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.05 0.08 0.08 0.03 -.03 -.10 -.17 "  /* B */
/* B */  "-.25 -.33 -.38 -.40 -.42 -.44 -.46 -.48 "  /* M */
/* M */  "-.50 -.50 -.50 -.52 -.55 -.50 -.40 -.30 "  /* R */
/* R */  "-.22 -.13 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 -.01 -.07 -.13 -.19 -.25 -.30 "  /* G */
/* G */  "-.32 -.34 -.36 -.34 -.28 -.21 -.14 -.07 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


static const char sp960_matte_sat_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.05 1.10 1.10 1.10 1.10 1.10 1.10 "  /* B */
/* B */  "1.10 1.10 1.10 1.10 1.10 1.10 1.10 1.10 "  /* M */
/* M */  "1.10 1.10 1.10 1.10 1.10 1.10 1.10 1.10 "  /* R */
/* R */  "1.10 1.10 1.10 1.05 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 1.05 1.10 1.10 1.10 1.10 "  /* G */
/* G */  "1.10 1.10 1.05 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char sp960_matte_lum_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.45 0.48 0.52 0.56 0.61 0.73 0.82 0.74 "  /* B */
/* B */  "0.68 0.68 0.68 0.68 0.72 0.80 0.95 1.05 "  /* M */
/* M */  "1.05 1.05 1.05 1.05 1.05 1.05 1.05 1.05 "  /* R */
/* R */  "1.05 1.05 1.05 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 0.99 0.92 0.72 0.61 0.54 "  /* G */
/* G */  "0.48 0.46 0.46 0.46 0.46 0.46 0.46 0.46 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char sp960_matte_hue_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.05 0.08 0.08 0.03 -.03 -.10 -.17 "  /* B */
/* B */  "-.25 -.33 -.38 -.40 -.42 -.44 -.46 -.48 "  /* M */
/* M */  "-.50 -.50 -.50 -.52 -.55 -.50 -.40 -.30 "  /* R */
/* R */  "-.22 -.13 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 -.01 -.07 -.13 -.19 -.25 -.30 "  /* G */
/* G */  "-.32 -.34 -.36 -.34 -.28 -.21 -.14 -.07 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


static const char sp960_pgpp_sat_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.10 1.20 "  /* M */
/* M */  "1.20 1.20 1.20 1.20 1.20 1.20 1.20 1.20 "  /* R */
/* R */  "1.20 1.20 1.15 1.10 1.05 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 1.10 1.20 1.30 1.40 1.50 "  /* G */
/* G */  "1.50 1.40 1.30 1.20 1.10 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char sp960_pgpp_lum_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.49 0.52 0.58 0.65 0.73 0.77 0.77 0.75 "  /* B */
/* B */  "0.69 0.69 0.72 0.80 0.88 0.97 1.10 1.25 "  /* M */
/* M */  "1.25 1.25 1.22 1.18 1.15 1.15 1.15 1.15 "  /* R */
/* R */  "1.15 1.15 1.12 1.09 1.06 1.03 1.02 1.01 "  /* Y */
/* Y */  "1.00 1.00 1.00 0.99 0.92 0.80 0.68 0.61 "  /* G */
/* G */  "0.54 0.54 0.54 0.54 0.53 0.53 0.52 0.50 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char sp960_pgpp_hue_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.05 0.08 0.08 0.03 -.03 -.10 -.17 "  /* B */
/* B */  "-.25 -.33 -.38 -.40 -.42 -.44 -.46 -.48 "  /* M */
/* M */  "-.50 -.50 -.50 -.52 -.55 -.50 -.40 -.30 "  /* R */
/* R */  "-.22 -.13 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 -.01 -.07 -.13 -.19 -.25 -.30 "  /* G */
/* G */  "-.32 -.34 -.36 -.34 -.28 -.21 -.14 -.07 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char ultra_photo_sat_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.20 1.10 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
/* R */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 1.00 1.00 1.10 1.20 1.30 "  /* G */
/* G */  "1.35 1.35 1.35 1.30 1.30 1.25 1.20 1.20 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char ultra_photo_lum_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.27 0.35 0.44 0.52 0.58 0.63 0.65 0.65 "  /* B */
/* B */  "0.60 0.60 0.63 0.67 0.74 0.82 0.88 0.93 "  /* M */
/* M */  "0.97 0.97 0.97 0.97 0.96 0.95 0.95 0.95 "  /* R */
/* R */  "0.95 0.96 0.97 0.98 0.99 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 0.90 0.80 0.70 0.60 0.51 0.45 0.41 "  /* G */
/* G */  "0.39 0.36 0.33 0.30 0.28 0.27 0.27 0.27 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char ultra_photo_hue_adj[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.08 0.16 0.23 0.23 0.15 0.08 -.02 "  /* B */
/* B */  "-.12 -.22 -.32 -.36 -.36 -.34 -.34 -.34 "  /* M */
/* M */  "-.34 -.36 -.38 -.40 -.45 -.40 -.30 -.20 "  /* R */
/* R */  "-.12 -.07 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 0.00 -.05 -.10 -.15 -.22 -.24 "  /* G */
/* G */  "-.26 -.30 -.33 -.28 -.25 -.20 -.13 -.06 "  /* C */
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
  { "Plain", 0.615, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "PlainFast", 0.615, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Postcard", 0.83, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyFilm", 1.00, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Transparency", 1.00, .75, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Envelope", 0.615, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "BackFilm", 1.00, .75, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Matte", 0.85, .8, 1.0, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "MatteHeavy", 1.0, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Inkjet", 0.85, .5, 1, .10, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Coated", 1.10, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Photo", 1.00, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyPhoto", 1.10, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Semigloss", 1.00, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Luster", 1.00, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyPaper", 1.00, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Ilford", 1.0, 1.0, 1, .15, 1.35, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj  },
  { "ColorLife", 1.00, 1.0, 1, .15, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Other", 0.615, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
};

DECLARE_PAPER_ADJUSTMENTS(standard);

static const paper_adjustment_t photo_adjustments[] =
{
  { "Plain", 0.615, .5, 1, .1, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "PlainFast", 0.615, .5, 1, .1, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Postcard", 0.83, .5, 1, .1, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyFilm", 1.00, 1.0, 1, .2, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Transparency", 1.00, .75, 1, .2, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Envelope", 0.615, .5, 1, .1, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "BackFilm", 1.00, .75, 1, .2, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Matte", 0.85, .8, 1.0, .2, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "MatteHeavy", 1.0, 1.0, 1, .35, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Inkjet", 0.85, .375, 1, .2, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Coated", 1.10, 1.0, 1, .35, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Photo", 1.00, 1.00, 1, .35, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyPhoto", 1.10, 1.0, 1, .35, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Semigloss", 1.00, 1.0, 1, .35, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Luster", 1.00, 1.0, 1, .35, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyPaper", 1.00, 1.0, 1, .35, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Ilford", 1.0, 1.0, 1, .35, 1.35, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj  },
  { "ColorLife", 1.00, 1.0, 1, .35, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Other", 0.615, .5, 1, .1, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
};

DECLARE_PAPER_ADJUSTMENTS(photo);

static const paper_adjustment_t photo2_adjustments[] =
{
  { "Plain", 0.615, .5, 0.5, .1, .9, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "PlainFast", 0.615, .5, 0.5, .1, .9, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Postcard", 0.83, .5, 0.5, .1, .9, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "GlossyFilm", 1.00, 1.0, 0.5, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Transparency", 1.00, .75, 0.25, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Envelope", 0.615, .5, 0.5, .1, .9, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "BackFilm", 1.00, .75, 0.25, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Matte", 0.85, .8, 0.25, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "MatteHeavy", 0.85, 1.0, 0.25, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Inkjet", 0.85, .5, 0.5, .15, .9, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Coated", 1.00, 1.0, 0.25, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Photo", 1.00, 1.0, 0.25, .30, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "GlossyPhoto", 1.00, 1.0, 0.25, .3, .999, .86, 1, .99, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Semigloss", 1.00, 1.0, 0.25, .3, .999, .86, 1, .99, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Luster", 1.00, 1.0, 0.25, .3, .999, .86, 1, .99, 1, 1, 1,
    photo2_hue_adj, photo2_luster_lum_adj, photo2_sat_adj },
  { "GlossyPaper", 1.00, 1.0, 0.25, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Ilford", .85, 1.0, 0.25, .2, .999, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj  },
  { "ColorLife", 1.00, 1.0, 0.25, .2, .9, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
  { "Other", 0.615, .5, 0.5, .1, .9, 1, 1, 1, 1, 1, 1,
    photo2_hue_adj, photo2_lum_adj, photo2_sat_adj },
};

DECLARE_PAPER_ADJUSTMENTS(photo2);

static const paper_adjustment_t sp960_adjustments[] =
{
  { "Plain",        0.86, .2,  0.4, .1,   .9,   .9, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
  { "PlainFast",    0.86, .2,  0.4, .1,   .9,   1, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
  { "Postcard",     0.90, .2,  0.4, .1,   .9,   .9, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
  { "GlossyFilm",   0.9,  .3,  0.4, .2,   .999, 1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Transparency", 0.9,  .2,  0.4, .1,   .9,   1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Envelope",     0.86, .2,  0.4, .1,   .9,   1, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
  { "BackFilm",     0.9,  .2,  0.4, .1,   .9,   1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Matte",        0.9,  .25, 0.4, .2,   .9,   1, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
  { "MatteHeavy",   0.9,  .3,  0.4, .2,   .999, 1, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
  { "Inkjet",       0.9,  .2,  0.4, .15,  .9,   1, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
  { "Coated",       0.9,  .3,  0.4, .2,   .999, 1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Photo",        0.9,  .3,  0.4, .2,   .999, 1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "GlossyPhoto",  0.9,  .3,  0.4, .2,   .999, 1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Semigloss",    0.9,  .3,  0.4, .2,   .999, 1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Luster",       0.9,  .3,  0.4, .2,   .999, 1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "GlossyPaper",  0.9,  .3,  0.4, .15,  .9,   1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Ilford",       0.85, .3,  0.4, .15, 1.35,  1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj  },
  { "ColorLife",    0.9,  .3,  0.4, .15,  .9,   1, 1, 1, 1, 1, 1,
    sp960_hue_adj, sp960_lum_adj, sp960_sat_adj },
  { "Other",        0.86, .2,  0.4, .1,   .9,   1, 1, 1, 1, 1, 1,
    sp960_matte_hue_adj, sp960_matte_lum_adj, sp960_matte_sat_adj },
};

DECLARE_PAPER_ADJUSTMENTS(sp960);

static const paper_adjustment_t ultrachrome_photo_adjustments[] =
{
  { "Plain", 0.615, .5, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "PlainFast", 0.615, .5, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Postcard", 0.83, .5, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "GlossyFilm", 1.00, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Transparency", 1.00, .75, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Envelope", 0.615, .5, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "BackFilm", 1.00, .75, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Matte", 0.85, 0.8, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "MatteHeavy", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Inkjet", 0.85, .5, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Coated", 1.10, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Photo", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "GlossyPhoto", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Semigloss", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Luster", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "ArchivalMatte", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "WaterColorRadiant", 0.765, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "GlossyPaper", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Ilford", .85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj  },
  { "ColorLife", 0.85, 1.0, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Other", 0.615, .5, 1, .01, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
};

DECLARE_PAPER_ADJUSTMENTS(ultrachrome_photo);

static const paper_adjustment_t ultrachrome_matte_adjustments[] =
{
  { "Plain", 0.615, .5, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "PlainFast", 0.615, .5, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Postcard", 0.83, .5, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "GlossyFilm", 1.00, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Transparency", 1.00, .75, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Envelope", 0.615, .5, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "BackFilm", 1.00, .75, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Matte", 0.85, 0.8, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "MatteHeavy", 0.85, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Inkjet", 0.85, .5, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Coated", 1.10, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Photo", 1.00, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "GlossyPhoto", 0.85, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Semigloss", 0.85, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "ArchivalMatte", 0.94, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "WaterColorRadiant", 0.85, 1.0, 1, .02, 1.5, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Luster", 1.00, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "GlossyPaper", 1.00, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Ilford", .85, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj  },
  { "ColorLife", 1.00, 1.0, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
  { "Other", 0.615, .5, 1, 0, 1.25, 1, 1, 1, 1, 1, 1,
    ultra_photo_hue_adj, ultra_photo_lum_adj, ultra_photo_sat_adj },
};

DECLARE_PAPER_ADJUSTMENTS(ultrachrome_matte);

static const paper_adjustment_t durabrite_adjustments[] =
{
  { "Plain", 0.769, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "PlainFast", 0.769, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Postcard", 0.875, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyFilm", .875, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Transparency", .875, .75, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Envelope", 0.769, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "BackFilm", 1.0, .75, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Matte", 0.875, 0.8, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "MatteHeavy", 0.875, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Inkjet", 0.875, .5, 1, .10, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Coated", 1.0, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Photo", 1.0, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyPhoto", 0.875, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Semigloss", 0.875, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Luster", 0.875, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "GlossyPaper", 1.0, 1.0, 1, .15, .999, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Ilford", .875, 1.0, 1, .15, 1.35, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj  },
  { "ColorLife", .875, 1.0, 1, .15, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
  { "Other", 0.769, .5, 1, .075, .9, 1, 1, 1, 1, 1, 1,
    standard_hue_adj, standard_lum_adj, standard_sat_adj },
};

DECLARE_PAPER_ADJUSTMENTS(durabrite);

static const paper_t standard_papers[] =
{
  { "Plain", N_("Plain Paper"), PAPER_PLAIN,
    1, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "PlainFast", N_("Plain Paper Fast Load"), PAPER_PLAIN,
    5, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Postcard", N_("Postcard"), PAPER_PLAIN,
    2, 0, 0x00, 0x00, 0x02, NULL, NULL },
  { "GlossyFilm", N_("Glossy Film"), PAPER_PHOTO,
    3, 0, 0x6d, 0x00, 0x01, NULL, NULL },
  { "Transparency", N_("Transparencies"), PAPER_TRANSPARENCY,
    3, 0, 0x6d, 0x00, 0x02, NULL, NULL },
  { "Envelope", N_("Envelopes"), PAPER_PLAIN,
    4, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "BackFilm", N_("Back Light Film"), PAPER_TRANSPARENCY,
    6, 0, 0x6d, 0x00, 0x01, NULL, NULL },
  { "Matte", N_("Matte Paper"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL, NULL },
  { "MatteHeavy", N_("Matte Paper Heavyweight"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL, NULL },
  { "Inkjet", N_("Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Coated", N_("Photo Quality Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Photo", N_("Photo Paper"), PAPER_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL, NULL },
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, NULL },
  { "Semigloss", N_("Premium Semigloss Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, NULL },
  { "Luster", N_("Premium Luster Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, NULL },
  { "GlossyPaper", N_("Photo Quality Glossy Paper"), PAPER_PREMIUM_PHOTO,
    6, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Ilford", N_("Ilford Heavy Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, NULL },
  { "ColorLife", N_("ColorLife Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL, NULL },
  { "Other", N_("Other"), PAPER_PLAIN,
    0, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
};

DECLARE_PAPERS(standard);

static const paper_t durabrite_papers[] =
{
  { "Plain", N_("Plain Paper"), PAPER_PLAIN,
    1, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "PlainFast", N_("Plain Paper Fast Load"), PAPER_PLAIN,
    5, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Postcard", N_("Postcard"), PAPER_PLAIN,
    2, 0, 0x00, 0x00, 0x02, NULL, NULL },
  { "GlossyFilm", N_("Glossy Film"), PAPER_PHOTO,
    3, 0, 0x6d, 0x00, 0x01, NULL, NULL },
  { "Transparency", N_("Transparencies"), PAPER_TRANSPARENCY,
    3, 0, 0x6d, 0x00, 0x02, NULL, NULL },
  { "Envelope", N_("Envelopes"), PAPER_PLAIN,
    4, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "BackFilm", N_("Back Light Film"), PAPER_TRANSPARENCY,
    6, 0, 0x6d, 0x00, 0x01, NULL, NULL },
  { "Matte", N_("Matte Paper"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL, NULL },
  { "MatteHeavy", N_("Matte Paper Heavyweight"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL, NULL },
  { "Inkjet", N_("Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Coated", N_("Photo Quality Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Photo", N_("Photo Paper"), PAPER_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL, NULL },
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, "RGB", NULL },
  { "Semigloss", N_("Premium Semigloss Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, "RGB", NULL },
  { "Luster", N_("Premium Luster Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, "RGB", NULL },
  { "GlossyPaper", N_("Photo Quality Glossy Paper"), PAPER_PHOTO,
    6, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
  { "Ilford", N_("Ilford Heavy Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, NULL },
  { "ColorLife", N_("ColorLife Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL, NULL },
  { "Other", N_("Other"), PAPER_PLAIN,
    0, 0, 0x6b, 0x1a, 0x01, NULL, NULL },
};

DECLARE_PAPERS(durabrite);

static const paper_t ultrachrome_papers[] =
{
  { "Plain", N_("Plain Paper"), PAPER_PLAIN,
    1, 0, 0x6b, 0x1a, 0x01, NULL, "UltraMatte" },
  { "PlainFast", N_("Plain Paper Fast Load"), PAPER_PLAIN,
    5, 0, 0x6b, 0x1a, 0x01, NULL, "UltraMatte" },
  { "Postcard", N_("Postcard"), PAPER_PLAIN,
    2, 0, 0x00, 0x00, 0x02, NULL, "UltraMatte" },
  { "GlossyFilm", N_("Glossy Film"), PAPER_PHOTO,
    3, 0, 0x6d, 0x00, 0x01, NULL, "UltraPhoto" },
  { "Transparency", N_("Transparencies"), PAPER_TRANSPARENCY,
    3, 0, 0x6d, 0x00, 0x02, NULL, "UltraPhoto" },
  { "Envelope", N_("Envelopes"), PAPER_PLAIN,
    4, 0, 0x6b, 0x1a, 0x01, NULL, "UltraMatte" },
  { "BackFilm", N_("Back Light Film"), PAPER_TRANSPARENCY,
    6, 0, 0x6d, 0x00, 0x01, NULL, "UltraPhoto" },
  { "Matte", N_("Matte Paper"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL, "UltraMatte" },
  { "MatteHeavy", N_("Matte Paper Heavyweight"), PAPER_GOOD,
    7, 0, 0x00, 0x00, 0x02, NULL, "UltraMatte" },
  { "Inkjet", N_("Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL, "UltraMatte" },
  { "Coated", N_("Photo Quality Inkjet Paper"), PAPER_GOOD,
    7, 0, 0x6b, 0x1a, 0x01, NULL, "UltraPhoto" },
  { "Photo", N_("Photo Paper"), PAPER_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL, "UltraPhoto" },
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, "UltraPhoto" },
  { "Semigloss", N_("Premium Semigloss Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, "UltraPhoto" },
  { "Luster", N_("Premium Luster Photo Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, "UltraPhoto" },
  { "ArchivalMatte", N_("Archival Matte Paper"), PAPER_PREMIUM_PHOTO,
    7, 0, 0x00, 0x00, 0x02, NULL, "UltraMatte" },
  { "WaterColorRadiant", N_("Watercolor Paper - Radiant White"), PAPER_PREMIUM_PHOTO,
    7, 0, 0x00, 0x00, 0x02, NULL, "UltraMatte" },
  { "GlossyPaper", N_("Photo Quality Glossy Paper"), PAPER_PHOTO,
    6, 0, 0x6b, 0x1a, 0x01, NULL, "UltraPhoto" },
  { "Ilford", N_("Ilford Heavy Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x80, 0x00, 0x02, NULL, "UltraMatte" },
  { "ColorLife", N_("ColorLife Paper"), PAPER_PREMIUM_PHOTO,
    8, 0, 0x67, 0x00, 0x02, NULL, "UltraPhoto" },
  { "Other", N_("Other"), PAPER_PLAIN,
    0, 0, 0x6b, 0x1a, 0x01, NULL, "UltraMatte" },
};

DECLARE_PAPERS(ultrachrome);
