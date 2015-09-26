/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *   Copyright (c) 2006 Sascha Sommer (saschasommer@freenet.de)
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

/* media related structs */


/* media slots */

typedef struct {
  const char* name;
  const char* text;
  unsigned char code;
} canon_slot_t;

typedef struct {
  const char *name;
  short count;
  const canon_slot_t *slots;
} canon_slotlist_t;

#define DECLARE_SLOTS(name)                           \
static const canon_slotlist_t name##_slotlist = {     \
  #name,                                              \
  sizeof(name##_slots) / sizeof(canon_slot_t),        \
  name##_slots                                        \
}


static const canon_slot_t canon_default_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 4 },
  { "Manual",     N_ ("Manual with Pause"), 0 },
  { "ManualNP",   N_ ("Manual without Pause"), 1 },
  { "Cassette",   N_ ("Cassette"), 8 },
  { "CD",         N_ ("CD tray"), 10 },
};
DECLARE_SLOTS(canon_default);

/* Gernot: changes 2010-10-02 */
static const canon_slot_t canon_PIXMA_iP4000_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "CD",         N_ ("CD tray"), 0xa },
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },
};
DECLARE_SLOTS(canon_PIXMA_iP4000);

static const canon_slot_t canon_PIXMA_iP3100_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "CD",         N_ ("CD tray"), 0xa },
  { "AutoSwitch", N_ ("Continuous Autofeed"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },
};
DECLARE_SLOTS(canon_PIXMA_iP3100);

static const canon_slot_t canon_PIXMA_iP2000_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "Front",      N_ ("Front Feeder"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },
};
DECLARE_SLOTS(canon_PIXMA_iP2000);

static const canon_slot_t canon_MULTIPASS_MP530_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "CD",         N_ ("CD tray"), 0xa },
  { "AutoSwitch", N_ ("Continuous Autofeed"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },
};
DECLARE_SLOTS(canon_MULTIPASS_MP530);

static const canon_slot_t canon_MULTIPASS_MP150_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
};
DECLARE_SLOTS(canon_MULTIPASS_MP150);

static const canon_slot_t canon_MULTIPASS_MP250_slots[] = {
  { "Rear",       N_ ("Rear tray"), 0x4 },
};
DECLARE_SLOTS(canon_MULTIPASS_MP250);

static const canon_slot_t canon_PIXMA_MG2100_slots[] = {
  { "Front",      N_ ("Front tray"), 0x8 },
};
DECLARE_SLOTS(canon_PIXMA_MG2100);

/* MP710/740 have adjustment for thick media: gap=10 */
static const canon_slot_t canon_MULTIPASS_MP710_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "AutoThick",  N_ ("Auto Sheet Feeder (thick media)"), 0x4 },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_MULTIPASS_MP710);

static const canon_slot_t canon_MULTIPASS_MP900_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_MULTIPASS_MP900);

static const canon_slot_t canon_BJC_i860_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_BJC_i860);

static const canon_slot_t canon_BJC_S800_slots[] = {
  { "Auto",       N_ ("Auto Sheet Feeder"), 0x4 },
  { "HandFeed",   N_ ("Hand Feeding"), 0x1 },
};
DECLARE_SLOTS(canon_BJC_S800);

static const canon_slot_t canon_MULTIPASS_MX7600_slots[] = {
  { "Cassette",   N_ ("Cassette"), 0x8 },
};
DECLARE_SLOTS(canon_MULTIPASS_MX7600);

static const canon_slot_t canon_PIXMA_iX7000_slots[] = {
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "HandFeed",   N_ ("Hand Feeding"), 0x1 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
};
DECLARE_SLOTS(canon_PIXMA_iX7000);

static const canon_slot_t canon_PIXMA_iP4500_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed"), 0xf }, /* no paper automatic change source*/
  { "CD",         N_ ("CD tray"), 0xa },
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 15 },
};
DECLARE_SLOTS(canon_PIXMA_iP4500);

static const canon_slot_t canon_MULTIPASS_MX850_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed (both)"), 0xf }, /* no paper automatic change source*/
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_MULTIPASS_MX850);

static const canon_slot_t canon_MULTIPASS_MP520_slots[] = {
  { "SelectKey",  N_ ("Selected by Paper Select Key"), 0x3 },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Front",      N_ ("Front tray"), 0x8 },
  /* There is also a "plain media only Front" option, but it seems to have same 0x8 when used, no idea whether it should take different values */
};
DECLARE_SLOTS(canon_MULTIPASS_MP520);

static const canon_slot_t canon_PIXMA_iP3500_slots[] = {
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Front",      N_ ("Front tray"), 0x8 },
  /* There is also a "plain media only Front" option, but it seems to have same 0x8 when used, no idea whether it should take different values */
};
DECLARE_SLOTS(canon_PIXMA_iP3500);

static const canon_slot_t canon_PIXMA_iP3600_slots[] = {
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xe },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed"), 0xf },
};
DECLARE_SLOTS(canon_PIXMA_iP3600);

static const canon_slot_t canon_PIXMA_iP4600_slots[] = {
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xe },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_PIXMA_iP4600);

/* Pro9000, Pro9500 series */
static const canon_slot_t canon_PIXMA_Pro9000_slots[] = {
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Front",      N_ ("Front tray"), 0xb },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_PIXMA_Pro9000);

static const canon_slot_t canon_PIXMA_MG5100_slots[] = {
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xe },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },
};
DECLARE_SLOTS(canon_PIXMA_MG5100);

static const canon_slot_t canon_PIXMA_MG5200_slots[] = {
  { "AutoSwitch", N_ ("Automatic Paper Source Switching"), 0xe },
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "Cassette",   N_ ("Cassette"), 0x8 },
  { "Continuous", N_ ("Continuous Autofeed"), 0xf },
  { "AllocPaper", N_ ("PaperAllocation"), 0x15 },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_PIXMA_MG5200);

/* Casssette containing an upper (cassette 1) and a lower tray (cassette 2) */
static const canon_slot_t canon_PIXMA_MG5400_slots[] = {
  { "Cassette",   N_ ("Cassette"), 0xd },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_PIXMA_MG5400);

/* Casssette containing an upper (cassette 1) and a lower tray (cassette 2) */
static const canon_slot_t canon_PIXMA_MX720_slots[] = {
  { "Cassette",   N_ ("Cassette"), 0xd },
};
DECLARE_SLOTS(canon_PIXMA_MX720);

static const canon_slot_t canon_PIXMA_iP8700_slots[] = {
  { "Rear",       N_ ("Rear tray"), 0x4 },
  { "CD",         N_ ("CD tray"), 0xa },
};
DECLARE_SLOTS(canon_PIXMA_iP8700);

static const canon_slot_t canon_MAXIFY_iB4000_slots[] = {
  { "Auto", N_ ("Cassette (Auto)"), 0xd },
  { "Cassette1",   N_ ("Cassette 1"), 0x8 },
  { "Cassette2",   N_ ("Cassette 2"), 0x9 },
};
DECLARE_SLOTS(canon_MAXIFY_iB4000);

/* media types */

typedef struct {
  const char *name;                        /* Internal Name may not contain spaces */
  const char *text;                        /* Translateable name */
  unsigned char media_code_c;              /* Media Code used for the ESC (c (SetColor) command */
  unsigned char media_code_l;              /* Media Code used for the ESC (l (SetTray) command */
  unsigned char media_code_P;              /* Media Code used for the ESC (P (Unknown) command  */
  unsigned char media_code_w;              /* Media Code used for the ESC (w (Unknown) command  */
  double base_density;
  double k_lower_scale;
  double k_upper;
  const char *hue_adjustment;
  const char *lum_adjustment;
  const char *sat_adjustment;
} canon_paper_t;

typedef struct {
  const char *name;
  short count;
  const canon_paper_t *papers;
} canon_paperlist_t;

#define DECLARE_PAPERS(name)                            \
static const canon_paperlist_t name##_paperlist = {     \
  #name,                                                \
  sizeof(name##_papers) / sizeof(canon_paper_t),        \
  name##_papers                                         \
}


/* paperlists for the various printers. The first entry will be the default */

static const canon_paper_t canon_default_papers[] = { /*                        k_lower    hue_adj sat_adj */
  /* Name                    Text                        (c   (l   (P   (w Density     k_upper lum_adj     */
  { "Plain",		N_ ("Plain Paper"),		0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		0x02,0x02,0x00,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "BackPrint",	N_ ("Back Print Film"),		0x03,0x03,0x00,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Fabric",		N_ ("Fabric Sheets"),		0x04,0x04,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),		0x08,0x08,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),	0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),	0x03,0x03,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		0x06,0x06,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),	0x05,0x05,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyCard",	N_ ("Glossy Photo Cards"),	0x0a,0x0a,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),		0x09,0x09,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "Other",		N_ ("Other"),                   0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_default);

static const canon_paper_t canon_PIXMA_iP4000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CD",		N_ ("CD"),				0x00,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },  /* legacy */
  /* FIXME media code for c) should be 0x0c for CD but this will restrict CD printing to a single, not well supported, resolution */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* added 2011-11-26 */
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* added 2011-11-26 */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
};
DECLARE_PAPERS(canon_PIXMA_iP4000);

static const canon_paper_t canon_PIXMA_iP2000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP2000);

static const canon_paper_t canon_PIXMA_iP3000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP3000);

static const canon_paper_t canon_PIXMA_iP3100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_PIXMA_iP3100);

static const canon_paper_t canon_PIXMA_MP5_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		        0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0b 0x11 */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0d 0x09 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_PIXMA_MP5);

static const canon_paper_t canon_PIXMA_MP55_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		        0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0b 0x11 */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0a 0x10 */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x07,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0d 0x09 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_PIXMA_MP55);

static const canon_paper_t canon_PIXMA_MPC400_papers[] = {
  { "Plain",		N_ ("Plain Paper"),                     0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),	     	0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),	     	0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),		     	0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested: using different from 0x0d 0x09 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested: using different from 0x0d 0x09 */
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyCard",	N_ ("Glossy Photo Cards"),	        0x05,0x0c,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),	     	0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),		     	0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_PIXMA_MPC400);

static const canon_paper_t canon_MULTIPASS_MP900_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photopaper Plus Double Sided"), 0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",  N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",  N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
};
DECLARE_PAPERS(canon_MULTIPASS_MP900);

static const canon_paper_t canon_PIXMA_iP4100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
};
DECLARE_PAPERS(canon_PIXMA_iP4100);

static const canon_paper_t canon_PIXMA_iP4200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  /* FIXME media code for c) should be 0x0c for CD but this will restrict CD printing to an unsupported resolution */
  { "CD",		N_ ("CD"),				0x00,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* added 2011-11-26 */
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* added 2011-11-26 */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP4200);

static const canon_paper_t canon_BJC_S200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_S200);

static const canon_paper_t canon_BJC_S300_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		        0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested*/
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyCard",	N_ ("Glossy Photo Cards"),	        0x05,0x0c,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },/* different from expected 0xa 0xa */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_S300);

static const canon_paper_t canon_BJC_S330_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		        0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0b 0x11 */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_S330);

static const canon_paper_t canon_BJC_S520_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		        0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0b 0x11 */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0a 0x10 */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_S520);

static const canon_paper_t canon_BJC_S750_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		        0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0b 0x11 */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0a 0x10 */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_S750);

static const canon_paper_t canon_BJC_S800_papers[] = {
  { "Plain",		N_ ("Plain Paper"),		        0x00,0x00,0x00,0x00,0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0b 0x11 */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0x0a 0x10 */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyCard",	N_ ("Glossy Photo Cards"),	        0x05,0x0a,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },/* different from expected 0xa 0xa */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x00,0x0f,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0xf 0x14 */
};
DECLARE_PAPERS(canon_BJC_S800);

static const canon_paper_t canon_BJC_i50_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i50);

static const canon_paper_t canon_BJC_i80_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_BJC_i80);

static const canon_paper_t canon_BJC_i250_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 11 */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* 250i,350i different from expected 08 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* 250i,350i */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i250);

static const canon_paper_t canon_BJC_i320_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i320);

static const canon_paper_t canon_BJC_i450_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i450);

static const canon_paper_t canon_BJC_i455_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i455);

static const canon_paper_t canon_BJC_i550_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x07,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected d */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i550);

static const canon_paper_t canon_BJC_i560_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i560);

static const canon_paper_t canon_BJC_i850_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x07,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected d 9 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i850);

static const canon_paper_t canon_BJC_i950_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.999, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x09,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected d 9 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i950);

static const canon_paper_t canon_BJC_i6100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected b 11 */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected a 10 */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x07,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected d */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_BJC_i6100);

static const canon_paper_t canon_BJC_i9100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected b 11 */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected a 10 */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x09,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected d 09 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i9100);

static const canon_paper_t canon_BJC_i9900_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_BJC_i9900);

static const canon_paper_t canon_PIXMA_iP90_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP90);

static const canon_paper_t canon_PIXMA_iP100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP100);

static const canon_paper_t canon_PIXMA_iP1000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*experiment*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*experiment*/
};
DECLARE_PAPERS(canon_PIXMA_iP1000);

static const canon_paper_t canon_PIXMA_iP1200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP1200);

static const canon_paper_t canon_PIXMA_iP1500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* experimental */
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 1.00, 0.900, 0, 0, 0 },/* experimental */
};
DECLARE_PAPERS(canon_PIXMA_iP1500);

static const canon_paper_t canon_PIXMA_iP2200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP2200);

static const canon_paper_t canon_PIXMA_iP2600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP2600);

static const canon_paper_t canon_PIXMA_iP3300_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPpro*/
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPsuper */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPsuperDS */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPgloss */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPmatte */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPother */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* HiRes */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjetHagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
};
DECLARE_PAPERS(canon_PIXMA_iP3300);

static const canon_paper_t canon_PIXMA_iP5000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photopaper Plus Double Sided"), 0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*untested*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*untested*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP5000);

static const canon_paper_t canon_PIXMA_iP6100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photopaper Plus Double Sided"), 0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CD",		N_ ("CD"),				0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, 
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP6100);

static const canon_paper_t canon_PIXMA_iP6600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP6600);

static const canon_paper_t canon_PIXMA_iP6700_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different from usual 13,1b,29 */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP6700);

static const canon_paper_t canon_PIXMA_iP7100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x02,0x0d,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different c,l from usual */
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x02,0x0d,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different c,l from usual */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_PIXMA_iP7100);

static const canon_paper_t canon_PIXMA_iP7100_limited_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",  N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x02,0x0d,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* note: different c,l from usual */
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x02,0x0d,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* note: different c,l from usual */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",  N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
};
DECLARE_PAPERS(canon_PIXMA_iP7100_limited);

static const canon_paper_t canon_PIXMA_iP7500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP7500);

static const canon_paper_t canon_PIXMA_iP8500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP8500);

static const canon_paper_t canon_PIXMA_iP8500_limited_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photopaper Plus Double Sided"), 0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",  N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,0.78, 0.25, 0.900, 0, 0, 0 },
  { "PhotopaperOther",  N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP8500_limited);

static const canon_paper_t canon_PIXMA_iP9910_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x09,0x11,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different c,l from usual */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x09,0x0d,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different c,l from usual */
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x09,0x0d,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different c,l from usual */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP9910);

static const canon_paper_t canon_PIXMA_iP9910_limited_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x09,0x11,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* note: different c,l from usual */
  { "PhotopaperMatte",  N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x09,0x0d,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* note: different c,l from usual */
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x09,0x0d,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* note: different c,l from usual */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",  N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP9910_limited);

/* PIXMA Pro9000 */
static const canon_paper_t canon_PIXMA_Pro9000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /*PPsuper*/
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"), 	0x0b,0x11,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /*PPGgold*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /*PPgloss*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /*PPpro*/
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x09,0x0d,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /*PPGpro*/
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x09,0x11,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /*PPGproPlat*/
  { "PhotopaperMatte",  N_ ("Matte Photo Paper"),	        0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",  N_ ("Other Photo Paper"),	        0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x20,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* note: different c,l from usual */
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 }, /* note: different from usual 13,1b,29 */
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),      0x14,0x19,0x31,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9000);

/* PIXMA Pro9000 Mk.II */
static const canon_paper_t canon_PIXMA_Pro9000mk2_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPgloss*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PP kinumecho*/
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGpro*/
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGproPlat*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different from usual 13,1b,29 */
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),      0x14,0x19,0x31,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9000mk2);

/* PIXMA Pro9500 */
static const canon_paper_t canon_PIXMA_Pro9500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGpro*/
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGproPlat*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different from usual 13,1b,29 */
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),	0x14,0x19,0x31,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9500);

/* PIXMA Pro9500 Mk.II */
static const canon_paper_t canon_PIXMA_Pro9500mk2_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGgold*/
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*PPGproPlat*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  /* special papers */
  { "Boardpaper",	N_ ("Board Paper"),		        0x18,0x1d,0x2e,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Canvas",	        N_ ("Canvas"),		                0x19,0x1e,0x2d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x16,0x1b,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note: different from usual 13,1b,29 */
  { "FineArtPremiumMatte",N_ ("Fine Art Premium Matte"),	0x15,0x1a,0x2c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtMuseumEtching",N_ ("Fine Art Museum Etching"),	0x14,0x19,0x31,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_Pro9500mk2);

static const canon_paper_t canon_SELPHY_DS700_papers[] = {
  { "GlossyPro",	N_ ("Photo Paper Pro"),	        0x09,0x0d,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 0x0b,0x11,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },/*PPsuper*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),	0x0a,0x10,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),	0x05,0x05,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),		0x0d,0x09,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			0x08,0x09,0x00,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
};
DECLARE_PAPERS(canon_SELPHY_DS700);

static const canon_paper_t canon_SELPHY_DS810_papers[] = {
  { "GlossyPro",	N_ ("Photo Paper Pro"),	        0x09,0x0d,0x1a,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 0x0b,0x11,0x1d,0x00,1.00, 1.00, 0.999, 0, 0, 0 },/*PPsuper*/
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),	0x0a,0x10,0x1c,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),	0x05,0x05,0x16,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),		0x0d,0x09,0x1b,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			0x08,0x09,0x07,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
};
DECLARE_PAPERS(canon_SELPHY_DS810);

static const canon_paper_t canon_PIXMA_mini320_papers[] = {
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"),         0x0b,0x11,0x1d,0x00,1.00, 1.00, 0.999, 0, 0, 0 },/*PPsuper*/
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),	        0x0a,0x10,0x1c,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),	        0x05,0x05,0x16,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),      0x1d,0x23,0x32,0x00,1.00, 1.00, 0.999, 0, 0, 0 },/*PPGgold*/
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),		        0x0d,0x09,0x1b,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_mini320);

static const canon_paper_t canon_MULTIPASS_MP150_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP150);

static const canon_paper_t canon_MULTIPASS_MP190_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPpro */
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP190);

static const canon_paper_t canon_MULTIPASS_MP360_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x00,0x00,1.00, 1.00, 0.900, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP360);

/* MX300, MX310, MX700 */
static const canon_paper_t canon_MULTIPASS_MX300_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX300);

static const canon_paper_t canon_MULTIPASS_MX330_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX330);

/* MX340 --- MX350 is the same */
static const canon_paper_t canon_MULTIPASS_MX340_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX340);

/* MX360 --- MX410 is the same */
static const canon_paper_t canon_MULTIPASS_MX360_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX360);

static const canon_paper_t canon_MULTIPASS_MX420_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Semi-gloss"),	        0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX420);

static const canon_paper_t canon_MULTIPASS_MX850_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Semi-gloss"),	        0x0b,0x11,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho new !! */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX850);

static const canon_paper_t canon_MULTIPASS_MX880_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MX880);

static const canon_paper_t canon_MULTIPASS_MX7600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MX7600);

static const canon_paper_t canon_PIXMA_iX7000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iX7000);

static const canon_paper_t canon_MULTIPASS_MP240_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP240);

static const canon_paper_t canon_MULTIPASS_MP250_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "ProPhotoHagakiP",  N_ ("Hagaki P (Pro Photo)"),		0x1f,0x25,0x37,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* pro photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP250);

static const canon_paper_t canon_MULTIPASS_MP280_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP280);

static const canon_paper_t canon_MULTIPASS_MP470_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP470);

static const canon_paper_t canon_MULTIPASS_MP480_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
};
DECLARE_PAPERS(canon_MULTIPASS_MP480);

static const canon_paper_t canon_MULTIPASS_MP493_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "ProPhotoHagakiP",  N_ ("Hagaki P (Pro Photo)"),		0x1f,0x25,0x37,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Pro photo hagaki*/
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
};
DECLARE_PAPERS(canon_MULTIPASS_MP493);

static const canon_paper_t canon_MULTIPASS_MP520_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPpro*/
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPsuper */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPsuperDS */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPgloss */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPmatte */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPother */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* HiRes */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjetHagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
};
DECLARE_PAPERS(canon_MULTIPASS_MP520);

static const canon_paper_t canon_PIXMA_iP1900_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_iP1900);

static const canon_paper_t canon_MULTIPASS_MP600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CD",	        N_ ("CD"),	                        0x00,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*allow plain modes for now*/
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP600);

static const canon_paper_t canon_MULTIPASS_MP610_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP610);

static const canon_paper_t canon_MULTIPASS_MP630_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP630);

static const canon_paper_t canon_MULTIPASS_MP640_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "ProPhotoHagakiP",  N_ ("Hagaki P (Pro Photo)"),		0x1f,0x25,0x37,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* pro photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_MP640);

static const canon_paper_t canon_MULTIPASS_MP700_papers[] = {            /* no ESC (P for MP700/730 */
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photopaper Plus Double Sided"), 0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.999, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x07,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 07 09 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_MULTIPASS_MP700);

static const canon_paper_t canon_MULTIPASS_MP710_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photopaper Plus Double Sided"), 0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.999, 0, 0, 0 },/* untested */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x07,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* different from expected 0d 09 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* untested */
};
DECLARE_PAPERS(canon_MULTIPASS_MP710);

/* MP750/MP760/MP770/MP780/MP790 */
static const canon_paper_t canon_MULTIPASS_MP750_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Professional Photo Paper"),	0x09,0x0d,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Glossy Photo Paper Plus"), 	0x0b,0x11,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photopaper Plus Double Sided"), 0x10,0x15,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),		        0x06,0x06,0x00,0x00,0.78, 0.25, 0.999, 0, 0, 0 },/* untested */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* from Japanese models */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* from Japanese models */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency", 	N_ ("Transparencies"),			0x02,0x02,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* from US models */
};
DECLARE_PAPERS(canon_MULTIPASS_MP750);

static const canon_paper_t canon_PIXMA_iP2700_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),	        0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGpro */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGproPlat */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP kinumecho */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPG */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP matte */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hi res paper */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_iP2700);

static const canon_paper_t canon_PIXMA_iP3600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",	        N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",	        N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP3600);

static const canon_paper_t canon_PIXMA_iP4500_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (recommended)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (others)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP4500);

static const canon_paper_t canon_PIXMA_iP4600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",	        N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",	        N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP4600);

static const canon_paper_t canon_PIXMA_iP4700_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",	        N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "OtherPhotoHagakiO",N_ ("Hagaki O (Other Photo)"),		0x1f,0x25,0x37,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Other photo hagaki*/
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",   	N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_iP4700);

static const canon_paper_t canon_MULTIPASS_MP950_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),	                0x09,0x0d,0x1a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP950);

static const canon_paper_t canon_MULTIPASS_MP960_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),		        0x09,0x0d,0x1a,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble",N_ ("Photo Paper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",	        N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",	        N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),		        0x02,0x02,0x01,0x00,1.00, 0.25, 0.500, 0, 0, 0 }, /* untested */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),	        0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP960);

static const canon_paper_t canon_MULTIPASS_MP970_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),		        0x09,0x0d,0x1a,0x00,1.00, 1.00, 0.999, 0, 0, 0 },
  { "PhotopaperPlus",	N_ ("Photo Paper Plus Glossy"), 	0x0b,0x11,0x1d,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperPlusDouble", N_ ("Photopaper Plus Double Sided"),0x10,0x15,0x25,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",	        N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",	        N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Coated Photo Paper"),	0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP970);

static const canon_paper_t canon_MULTIPASS_MP980_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",	        N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP980);

static const canon_paper_t canon_MULTIPASS_MP990_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2", 	N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPro2",	N_ ("Photo Paper Pro II"),		0x1f,0x25,0x34,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",	        N_ ("T-Shirt Transfer"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",	        N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki",	N_ ("Ink Jet Hagaki"), 			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "ProPhotoHagakiP",  N_ ("Hagaki K (Pro Photo)"),		0x1f,0x25,0x37,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Pro photo hagaki*/
  { "Hagaki",		N_ ("Hagaki"),				0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope", 	N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_MULTIPASS_MP990);

static const canon_paper_t canon_PIXMA_MG2400_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },/* plain */
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed:  PPproLuster*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* glossy */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed:  all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed:  inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed:  Canon photo hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed:  hagaki */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed:  T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed:  PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG2400);

/* new paper type from October 2012 */
static const canon_paper_t canon_PIXMA_MG3200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPproLuster*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG3200);

static const canon_paper_t canon_PIXMA_P200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
};
DECLARE_PAPERS(canon_PIXMA_P200);

static const canon_paper_t canon_PIXMA_MG5100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG5100);

static const canon_paper_t canon_PIXMA_MG5200_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG5200);

/* new paper type from October 2012 */
static const canon_paper_t canon_PIXMA_MG5400_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPproLuster*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG5400);

static const canon_paper_t canon_PIXMA_MG6100_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG6100);

/* new paper type from October 2012 */
static const canon_paper_t canon_PIXMA_MG6300_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPproLuster*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),		0x05,0x05,0x16,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "CanonPhotoHagakiK",N_ ("Hagaki K (Canon Photo)"),		0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Canon photo hagaki*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki*/
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Untested */
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG6300);

/* new paper type from February 2014 */
static const canon_paper_t canon_PIXMA_MG2900_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed: PPproLuster*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGstandard */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed: all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed: inkjet hagaki */
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed: Canon photo hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed: hagaki */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed: T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unconfirmed: PP other */
};
DECLARE_PAPERS(canon_PIXMA_MG2900);
  
static const canon_paper_t canon_PIXMA_iP8700_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPproLuster */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGstandard */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Inkjet photo hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki */
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "FineArtPhotoRag",  N_ ("Fine Art Photo Rag"),	        0x13,0x18,0x28,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Untested */
  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_iP8700);

static const canon_paper_t canon_MULTIPASS_E400_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/*unsupported*/
};
DECLARE_PAPERS(canon_MULTIPASS_E400);

static const canon_paper_t canon_MULTIPASS_E480_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPproLuster */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGstandard */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported: all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported: inkjet hagaki */
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported: Inkjet photo hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported: hagaki */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported: T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_MULTIPASS_E480);

static const canon_paper_t canon_PIXMA_iX6800_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPproLuster */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGstandard */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* all hagaki */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* inkjet hagaki */
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* Inkjet photo hagaki */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* hagaki */
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* T-shirt */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* env */
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PP other */
};
DECLARE_PAPERS(canon_PIXMA_iX6800);

static const canon_paper_t canon_PIXMA_iP110_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x09,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* note Esc (l different */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
};
DECLARE_PAPERS(canon_PIXMA_iP110);

static const canon_paper_t canon_MAXIFY_iB4000_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGstandard */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki A (address side)"),	        0x08,0x09,0x38,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x00,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x00,0.78, 0.25, 0.500, 0, 0, 0 },/* unsupported */
};
DECLARE_PAPERS(canon_MAXIFY_iB4000);

/* new papers from September 2015 */
/* MG3600 does not use ESC (w command but MG5700, MG6800 do */
static const canon_paper_t canon_PIXMA_MG3600_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x03,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGgold */
  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x05,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x11,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x06,0.78, 0.25, 0.500, 0, 0, 0 },
  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x0e,0.78, 0.25, 0.500, 0, 0, 0 },/* PPGstandard */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x0f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x09,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagakiaddr", N_ ("Ink Jet Hagaki (A)"),		0x08,0x09,0x46,0x24,0.78, 0.25, 0.500, 0, 0, 0 },/* NEW Sep 2015 */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x0b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkjetPhotoHagakiKaddr",N_ ("Hagaki K (Inkjet Photo) (A)"),0x08,0x09,0x47,0x26,0.78, 0.25, 0.500, 0, 0, 0 },/* NEW Sep 2015 */
  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x0d,0.78, 0.25, 0.500, 0, 0, 0 },
  { "HagakiA", 	        N_ ("Hagaki (A)"),			0x08,0x09,0x48,0x28,0.78, 0.25, 0.500, 0, 0, 0 },/* Renamed Sep 2015 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x0c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x13,0.78, 0.25, 0.500, 0, 0, 0 },
};
DECLARE_PAPERS(canon_PIXMA_MG3600);

/* media using only modes with (unsupported) H ink commented out */
static const canon_paper_t canon_PIXMA_MG7700_papers[] = {
  { "Plain",		N_ ("Plain Paper"),			0x00,0x00,0x00,0x00,1.00, 0.25, 0.500, 0, 0, 0 },
  /*  { "PhotoPlusGloss2",  N_ ("Photo Paper Plus Glossy II"),	0x1d,0x23,0x32,0x03,0.78, 0.25, 0.500, 0, 0, 0 },*//* PPGgold */
  /*  { "PhotoProPlat",	N_ ("Photo Paper Pro Platinum"),	0x1e,0x24,0x33,0x05,0.78, 0.25, 0.500, 0, 0, 0 },*/
  /*  { "PhotoProLuster",	N_ ("Photo Paper Pro Luster"),		0x25,0x28,0x3f,0x11,0.78, 0.25, 0.500, 0, 0, 0 },*/
  /*  { "PhotoProSemiGloss",N_ ("Photo Paper Plus Semi-gloss"),	0x1a,0x1f,0x2a,0x06,0.78, 0.25, 0.500, 0, 0, 0 },*/
  /*  { "GlossyPaperStandard",N_ ("Photo Paper Glossy"),		0x05,0x05,0x44,0x0e,0.78, 0.25, 0.500, 0, 0, 0 },*//* PPGstandard */
  { "PhotopaperMatte",	N_ ("Matte Photo Paper"),		0x0a,0x10,0x1c,0x0f,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),		0x07,0x07,0x10,0x09,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkJetHagakiaddr", N_ ("Ink Jet Hagaki (A)"),		0x08,0x09,0x46,0x24,0.78, 0.25, 0.500, 0, 0, 0 },/* NEW Sep 2015 */
  { "InkJetHagaki", 	N_ ("Ink Jet Hagaki"),			0x0d,0x09,0x1b,0x0b,0.78, 0.25, 0.500, 0, 0, 0 },
  { "InkjetPhotoHagakiKaddr",N_ ("Hagaki K (Inkjet Photo) (A)"),0x08,0x09,0x47,0x26,0.78, 0.25, 0.500, 0, 0, 0 },/* NEW Sep 2015 */
  /*  { "InkjetPhotoHagakiK",N_ ("Hagaki K (Inkjet Photo)"),	0x05,0x05,0x36,0x0d,0.78, 0.25, 0.500, 0, 0, 0 },*/
  { "HagakiA", 	        N_ ("Hagaki (A)"),			0x08,0x09,0x48,0x28,0.78, 0.25, 0.500, 0, 0, 0 },/* Renamed Sep 2015 */
  { "Hagaki", 	        N_ ("Hagaki"),			        0x08,0x09,0x07,0x0c,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscCompat",	N_ ("Printable Disc (Compatible)"),	0x0c,0x12,0x1f,0x21,0.78, 0.25, 0.500, 0, 0, 0 },
  { "DiscOthers",	N_ ("Printable Disc (Other)"),		0x0c,0x12,0x20,0x22,0.78, 0.25, 0.500, 0, 0, 0 },
  /*  { "FineArtOther",     N_ ("Fine Art Other"),	                0x13,0x18,0x29,0x24,0.78, 0.25, 0.500, 0, 0, 0 },*/
  { "TShirt",		N_ ("T-Shirt Transfers"),		0x03,0x03,0x12,0x2a,0.78, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),			0x08,0x08,0x08,0x12,0.78, 0.25, 0.500, 0, 0, 0 },
  /*  { "PhotopaperOther",	N_ ("Other Photo Paper"),		0x0f,0x14,0x24,0x13,0.78, 0.25, 0.500, 0, 0, 0 },*/
};
DECLARE_PAPERS(canon_PIXMA_MG7700);

#endif
