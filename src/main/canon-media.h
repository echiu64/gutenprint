/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
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

/* This file contains the definitions for the possible Media Types
   TODO: Color Correction and Density adjustment 
*/

#ifndef GUTENPRINT_INTERNAL_CANON_MEDIA_H
#define GUTENPRINT_INTERNAL_CANON_MEDIA_H


#define DECLARE_PAPERS(name)                            \
static const canon_paperlist_t name##_paperlist =      \
{                                                       \
  #name,                                                \
  sizeof(name##_papers) / sizeof(canon_paper_t),              \
  name##_papers                                         \
}

static const canon_paper_t canon_default_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		0x00, 0x00,Q2,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		0x02, 0x02,Q2,1.00, 1.00, 0.900, 0, 0, 0 },
  { "BackPrint",	N_ ("Back Print Film"),		0x03, 0x03,Q2,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Fabric",		N_ ("Fabric Sheets"),		0x04, 0x04,Q2,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),		0x08, 0x08,Q2,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),	0x07, 0x07,Q2,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),	0x03, 0x03,Q2,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		0x06, 0x06,Q2,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),	0x05, 0x05,Q2,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyCard",	N_ ("Glossy Photo Cards"),	0x0a, 0x0a,Q2,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),		0x09, 0x09,Q2,1.00, 1.00, 0.999, 0, 0, 0 },
  { "Other",		N_ ("Other"),                   0x00, 0x00,Q2,0.50, 0.25, .5, 0, 0, 0 },
};
DECLARE_PAPERS(canon_default);

static const canon_paper_t canon_PIXMA_iP4000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,Q0|Q1|Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,Q2,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,Q2|Q3|Q4,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Photo Paper Matte"),		0x0a,0x10,Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,Q1|Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CD",		N_ ("CD"),				0x0c,0x12,Q2|Q3|Q4,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,Q2,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photoper Plus Double-Sided"),	0x10,0x15,Q2|Q3,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP4000);

#endif

