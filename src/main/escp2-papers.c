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
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"1.20;1.22;1.28;1.34;1.39;1.42;1.45;1.48;"  /* C */
"1.50;1.40;1.30;1.25;1.20;1.10;1.05;1.05;"  /* B */
"1.05;1.05;1.05;1.05;1.05;1.05;1.05;1.05;"  /* M */
"1.05;1.05;1.05;1.10;1.10;1.10;1.10;1.10;"  /* R */
"1.10;1.15;1.30;1.45;1.60;1.75;1.90;2.00;"  /* Y */
"2.10;2.00;1.80;1.70;1.60;1.50;1.40;1.30;"; /* G */

static const char pgpp_sat_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* C */
"1.00;1.00;1.00;1.03;1.05;1.07;1.09;1.11;"  /* B */
"1.13;1.13;1.13;1.13;1.13;1.13;1.13;1.13;"  /* M */
"1.13;1.10;1.05;1.00;1.00;1.00;1.00;1.00;"  /* R */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* Y */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"; /* G */

static const char pgpp_lum_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* C */
"1.00;1.00;1.00;1.03;1.05;1.07;1.09;1.11;"  /* B */
"1.13;1.13;1.13;1.13;1.13;1.13;1.13;1.13;"  /* M */
"1.13;1.10;1.05;1.00;1.00;1.00;1.00;1.00;"  /* R */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* Y */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"; /* G */

static const char pgpp_hue_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;-6.0;6.0:"
"0.00;0.00;0.00;0.00;0.00;0.01;0.02;0.03;"  /* C */
"0.05;0.05;0.05;0.04;0.04;0.03;0.02;0.01;"  /* B */
"0.00;-.03;-.05;-.07;-.09;-.11;-.13;-.14;"  /* M */
"-.15;-.13;-.10;-.06;-.04;-.02;-.01;0.00;"  /* R */
"0.00;0.00;0.00;0.00;0.00;0.00;0.00;0.00;"  /* Y */
"0.00;0.00;0.00;0.00;0.00;0.00;0.00;0.00;"; /* G */

static const paper_t standard_papers[] =
{
  { "Plain", N_("Plain Paper"),
    1, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "PlainFast", N_("Plain Paper Fast Load"),
    5, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Postcard", N_("Postcard"),
    2, 0, 0.83, .2, .6, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "GlossyFilm", N_("Glossy Film"),
    3, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Transparency", N_("Transparencies"),
    3, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.769, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "BackFilm", N_("Back Light Film"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, NULL, NULL},
  { "Matte", N_("Matte Paper"),
    7, 0, 0.85, 1.0, .999, 1.05, 1.0, 0.95, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, NULL, NULL},
  { "Inkjet", N_("Inkjet Paper"),
    7, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    7, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Photo", N_("Photo Paper"),
    8, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    8, 0, 1.10, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.03, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    8, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Ilford", N_("Ilford Heavy Paper"),
    8, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL },
  { "ColorLife", N_("ColorLife Paper"),
    8, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "Other", N_("Other"),
    0, 0, 0.769, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
};

const paperlist_t stpi_escp2_standard_paper_list =
{
  sizeof(standard_papers) / sizeof(paper_t),
  standard_papers
};

static const paper_t sp780_papers[] =
{
  { "Plain", N_("Plain Paper"),
    6, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "PlainFast", N_("Plain Paper Fast Load"),
    1, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Postcard", N_("Postcard"),
    3, 0, 0.83, .2, .6, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "GlossyFilm", N_("Glossy Film"),
    0, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Transparency", N_("Transparencies"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.769, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "BackFilm", N_("Back Light Film"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, NULL, NULL},
  { "Matte", N_("Matte Paper"),
    2, 0, 0.85, 1.0, .999, 1.05, .9, 1.05, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, NULL, NULL},
  { "Inkjet", N_("Inkjet Paper"),
    6, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    0, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Photo", N_("Photo Paper"),
    2, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    7, 0, 1.10, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.03, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    7, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Ilford", N_("Ilford Heavy Paper"),
    2, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL },
  { "ColorLife", N_("ColorLife Paper"),
    2, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "Other", N_("Other"),
    0, 0, 0.769, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
};

const paperlist_t stpi_escp2_sp780_paper_list =
{
  sizeof(sp780_papers) / sizeof(paper_t),
  sp780_papers
};

static const paper_t c80_papers[] =
{
  { "Plain", N_("Plain Paper"),
    1, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "PlainFast", N_("Plain Paper Fast Load"),
    5, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Postcard", N_("Postcard"),
    2, 0, 0.83, .2, .6, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "GlossyFilm", N_("Glossy Film"),
    3, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Transparency", N_("Transparencies"),
    3, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.769, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "BackFilm", N_("Back Light Film"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, NULL, NULL},
  { "Matte", N_("Matte Paper"),
    7, 0, 0.9, 1.0, .999, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, NULL, NULL},
  { "Inkjet", N_("Inkjet Paper"),
    7, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    7, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Photo", N_("Photo Paper"),
    8, 0, 1.20, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    8, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.03, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    8, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Ilford", N_("Ilford Heavy Paper"),
    8, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL },
  { "ColorLife", N_("ColorLife Paper"),
    8, 0, 1.20, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "Other", N_("Other"),
    0, 0, 0.769, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
};

const paperlist_t stpi_escp2_c80_paper_list =
{
  sizeof(c80_papers) / sizeof(paper_t),
  c80_papers
};

static const paper_t sp950_papers[] =
{
  { "Plain", N_("Plain Paper"),
    6, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "PlainFast", N_("Plain Paper Fast Load"),
    1, 0, 0.769, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Postcard", N_("Postcard"),
    3, 0, 0.83, .2, .6, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "GlossyFilm", N_("Glossy Film"),
    0, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Transparency", N_("Transparencies"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.769, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "BackFilm", N_("Back Light Film"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, NULL, NULL},
  { "Matte", N_("Matte Paper"),
    2, 0, 0.85, 1.0, .999, 1.05, .9, 1.05, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, NULL, NULL},
  { "Inkjet", N_("Inkjet Paper"),
    6, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    0, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Photo", N_("Photo Paper"),
    2, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    7, 0, 0.85, 1.0, .9, 0.9, 1.04, 0.93, 0.9, 1.04, 0.93,
    0.9, 1.0, 0x80, 0x00, 0x02,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    7, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Ilford", N_("Ilford Heavy Paper"),
    2, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL },
  { "ColorLife", N_("ColorLife Paper"),
    2, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "Other", N_("Other"),
    0, 0, 0.769, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
};

const paperlist_t stpi_escp2_sp950_paper_list =
{
  sizeof(sp950_papers) / sizeof(paper_t),
  sp950_papers
};
