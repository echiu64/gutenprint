/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *   Copyright (c) 2006 - 2007 Sascha Sommer (saschasommer@freenet.de)
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

/* This file contains the usage matrix matching media with modes
*/

#ifndef GUTENPRINT_INTERNAL_CANON_MEDIA_MODE_H
#define GUTENPRINT_INTERNAL_CANON_MEDIA_MODE_H

/* create matrix of which media are used with which mode */

typedef struct {
  const char *name; /* unstranslated media name */
  const char** mode_name_list; /* untranslated mode names */
  const unsigned int use_flags;
  /* flags to indicate support in the media */
#define INKSET_BLACK_SUPPORT 0x1
#define INKSET_COLOR_SUPPORT 0x2
  /*#define INKSET_BOTH_SUPPORT  0x4*/
#define INKSET_PHOTO_SUPPORT 0x8
  /* duplex support for media --- this is currently not implemented: so duplex can be specified for all media types */
#define DUPLEX_SUPPORT       0x10
  /* flags to indicate existence of special replacement modes to search for */
#define INKSET_BLACK_MODEREPL 0x100
#define INKSET_COLOR_MODEREPL 0x200
  /*#define INKSET_BOTH_MODEREPL  0x400*/
#define INKSET_PHOTO_MODEREPL 0x800
#define DUPLEX_MODEREPL       0x1000

} canon_modeuse_t;

typedef struct {
  const char *name;
  const short count;
  const canon_modeuse_t *modeuses;
} canon_modeuselist_t;

#define DECLARE_MODEUSES(name)                              \
  static const canon_modeuselist_t name##_modeuselist = {   \
  #name,                                                    \
  sizeof(name##_modeuses) / sizeof(canon_modeuse_t),        \
  name##_modeuses                                           \
}

/* Ordering of data: 
   BJC
   DS
   mini
   S
   i --- *i is Japanese model, with Hagaki/inkjetHagaki support. 
         i* is US model without such support. Not sure about European models.
   iP
   iX
   MP
   MX
   MG
   Pro
*/

/* ----------------------------------- Canon BJC 30 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_30_modeuses_plain[] = {
  "720x360dpi",
  "360x360dpi",
  "180x180dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_30_modeuses[] = {
  { "Plain",		canon_BJC_30_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_30_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_30_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_30_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_30_modeuses_plain, 0 },
  { "Coated",		canon_BJC_30_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_30_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_30_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_30_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_30_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_30_modeuses_plain, 0 },
  { "Other",		canon_BJC_30_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_30);

/* ----------------------------------- Canon BJC 85 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_85_modeuses_plain[] = {
  "720x360dpi",
  "360x360dmt",
  "360x360dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_85_modeuses[] = {
  { "Plain",		canon_BJC_85_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_85_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_85_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_85_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_85_modeuses_plain, 0 },
  { "Coated",		canon_BJC_85_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_85_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_85_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_85_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_85_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_85_modeuses_plain, 0 },
  { "Other",		canon_BJC_85_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_85);

/* ----------------------------------- Canon BJC 210 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_210_modeuses_plain[] = {
  "720x360dpi",
  "360x360dpi",
  "180x180dpi",
  "90x90dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_210_modeuses[] = {
  { "Plain",		canon_BJC_210_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_210_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_210_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_210_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_210_modeuses_plain, 0 },
  { "Coated",		canon_BJC_210_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_210_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_210_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_210_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_210_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_210_modeuses_plain, 0 },
  { "Other",		canon_BJC_210_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_210);

/* ----------------------------------- Canon BJC 240 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_240_modeuses_plain[] = {
  "720x360dpi",
  "360x360dmt",
  "360x360dpi",
  "180x180dpi",
  "90x90dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_240_modeuses[] = {
  { "Plain",		canon_BJC_240_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_240_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_240_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_240_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_240_modeuses_plain, 0 },
  { "Coated",		canon_BJC_240_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_240_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_240_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_240_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_240_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_240_modeuses_plain, 0 },
  { "Other",		canon_BJC_240_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_240);

/* ----------------------------------- Canon BJC 2000 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_2000_modeuses_plain[] = {
  "360x360dpi",
  "180x180dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_2000_modeuses[] = {
  { "Plain",		canon_BJC_2000_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_2000_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_2000_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_2000_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_2000_modeuses_plain, 0 },
  { "Coated",		canon_BJC_2000_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_2000_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_2000_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_2000_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_2000_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_2000_modeuses_plain, 0 },
  { "Other",		canon_BJC_2000_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_2000);

/* ----------------------------------- Canon BJC 3000 ----------------------------------- */

static const char* canon_BJC_3000_modeuses_plain[] = {
  "1440x720dpi",/*untested*/
  "720x720dpi",
  "360x360dpi",
  "360x360dmt",
  "360x360dpi_draft",
  "180x180dpi",
  /* Photo */
  "360x360dpi_photo",
  NULL
};

static const char* canon_BJC_3000_modeuses_PPgloss[] = {
  "1440x720dpi",
  "720x720dpi",
  "360x360dpi",
  "360x360dmt",
  /* Photo */
  "360x360dpi_photo",
  NULL
};

static const char* canon_BJC_3000_modeuses_coated[] = {
  "1440x720dpi",
  "720x720dpi",
  "360x360dpi",
  "360x360dmt",
  "360x360dpi_draft",
  /* Photo */
  "360x360dpi_photo",
  NULL
};

static const char* canon_BJC_3000_modeuses_glossFilm[] = {
  "720x720dpi",
  "360x360dpi",
  "360x360dmt",
  NULL
};

static const char* canon_BJC_3000_modeuses_Tshirt[] = {
  "360x360dpi",
  "360x360dmt",
  NULL
};

static const char* canon_BJC_3000_modeuses_Transparency[] = {
  "360x360dpi",
  "360x360dmt",
  "360x360dpi_draft",
  NULL
};

static const char* canon_BJC_3000_modeuses_Envelope[] = {
  "720x720dpi",
  "360x360dpi",
  "360x360dmt",
  "360x360dpi_draft",
  NULL
};

static const canon_modeuse_t canon_BJC_3000_modeuses[] = {
  { "Plain",		canon_BJC_3000_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "Transparency",	canon_BJC_3000_modeuses_Transparency, INKSET_BLACK_SUPPORT },
  { "BackPrint",	canon_BJC_3000_modeuses_Tshirt, 0 },
  { "Fabric",		canon_BJC_3000_modeuses_Tshirt, 0 },/*untested*/
  { "Envelope",		canon_BJC_3000_modeuses_Envelope, INKSET_BLACK_SUPPORT },
  { "Coated",		canon_BJC_3000_modeuses_coated, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "TShirt",		canon_BJC_3000_modeuses_Tshirt, 0 },
  { "GlossyFilm",	canon_BJC_3000_modeuses_glossFilm, 0 },
  { "GlossyPaper",	canon_BJC_3000_modeuses_PPgloss, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "GlossyCard",	canon_BJC_3000_modeuses_PPgloss, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "GlossyPro",	canon_BJC_3000_modeuses_PPgloss, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },/*untested*/
  { "Other",		canon_BJC_3000_modeuses_PPgloss, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_3000);

/* ----------------------------------- Canon BJC 4300 ----------------------------------- */

static const char* canon_BJC_4300_modeuses_plain[] = {
  "360x360dpi_high",
  "360x360dpi",
  "360x360dmt",
  "720x360dpi",
  "360x360dpi_draft",
  "180x180dpi",
  /* Photo */
  "360x360dpi_photo",
  NULL
};

static const char* canon_BJC_4300_modeuses_PPgloss[] = {
  "360x360dpi_high",
  "360x360dpi",
  "360x360dmt",
  "720x360dpi",/*mono*/
  /* Photo */
  "360x360dpi_photo",
  NULL
};

static const char* canon_BJC_4300_modeuses_coated[] = {
  "360x360dpi_high",
  "360x360dpi",
  "360x360dmt",
  "720x360dpi",
  "360x360dpi_draft",
  /* Photo */
  "360x360dpi_photo",
  NULL
};

static const char* canon_BJC_4300_modeuses_glossFilm[] = {
  "360x360dpi_high",
  "360x360dpi",
  "360x360dmt",
  "720x360dpi",/*mono*/
  NULL
};

static const canon_modeuse_t canon_BJC_4300_modeuses[] = {
  { "Plain",		canon_BJC_4300_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "Transparency",	canon_BJC_4300_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "BackPrint",	canon_BJC_4300_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "Fabric",		canon_BJC_4300_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "Envelope",		canon_BJC_4300_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "Coated",		canon_BJC_4300_modeuses_coated, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "TShirt",		canon_BJC_4300_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "GlossyFilm",	canon_BJC_4300_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "GlossyPaper",	canon_BJC_4300_modeuses_PPgloss, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "GlossyCard",	canon_BJC_4300_modeuses_PPgloss, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "GlossyPro",	canon_BJC_4300_modeuses_PPgloss, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },/*untested*/
  { "Other",		canon_BJC_4300_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
};

DECLARE_MODEUSES(canon_BJC_4300);

/* ----------------------------------- Canon BJC 4400 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_4400_modeuses_plain[] = {
  "720x360dpi",
  "360x360dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_4400_modeuses[] = {
  { "Plain",		canon_BJC_4400_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_4400_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_4400_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_4400_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_4400_modeuses_plain, 0 },
  { "Coated",		canon_BJC_4400_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_4400_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_4400_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_4400_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_4400_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_4400_modeuses_plain, 0 },
  { "Other",		canon_BJC_4400_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_4400);

/* ----------------------------------- Canon BJC 4550 ----------------------------------- */

static const char* canon_BJC_4550_modeuses_plain[] = {
  "720x360dpi_high",
  "360x360dpi_high",
  "720x360dpi",
  "360x360dpi",
  "180x180dpi",
  /* Photo */
  "360x360dpi_photo",
  NULL
};

static const char* canon_BJC_4550_modeuses_glossFilm[] = {
  "720x360dpi_high",
  "360x360dpi_high",
  "360x360dpi",/*untested*/
  "180x180dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_4550_modeuses[] = {
  { "Plain",		canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "Transparency",	canon_BJC_4550_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "BackPrint",	canon_BJC_4550_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "Fabric",		canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT },
  { "Envelope",		canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "Coated",		canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "TShirt",		canon_BJC_4550_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "GlossyFilm",	canon_BJC_4550_modeuses_glossFilm, INKSET_BLACK_SUPPORT },
  { "GlossyPaper",	canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
  { "GlossyCard",	canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },/*untested*/
  { "GlossyPro",	canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },/*untested*/
  { "Other",		canon_BJC_4550_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_PHOTO_SUPPORT | INKSET_PHOTO_MODEREPL },
};

DECLARE_MODEUSES(canon_BJC_4550);

/* ----------------------------------- Canon BJC 5500 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_5500_modeuses_plain[] = {
  "360x360dpi",
  "180x180dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_5500_modeuses[] = {
  { "Plain",		canon_BJC_5500_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_5500_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_5500_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_5500_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_5500_modeuses_plain, 0 },
  { "Coated",		canon_BJC_5500_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_5500_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_5500_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_5500_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_5500_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_5500_modeuses_plain, 0 },
  { "Other",		canon_BJC_5500_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_5500);

/* ----------------------------------- Canon BJC 6000 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_6000_modeuses_plain[] = {
  "1440x720dpi",
  "720x720dpi",
  "360x360dmt",
  "360x360dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_6000_modeuses[] = {
  { "Plain",		canon_BJC_6000_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_6000_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_6000_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_6000_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_6000_modeuses_plain, 0 },
  { "Coated",		canon_BJC_6000_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_6000_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_6000_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_6000_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_6000_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_6000_modeuses_plain, 0 },
  { "Other",		canon_BJC_6000_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_6000);

/* ----------------------------------- Canon BJC 7000 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_7000_modeuses_plain[] = {
  "1200x1200dpi",
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_7000_modeuses[] = {
  { "Plain",		canon_BJC_7000_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_7000_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_7000_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_7000_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_7000_modeuses_plain, 0 },
  { "Coated",		canon_BJC_7000_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_7000_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_7000_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_7000_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_7000_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_7000_modeuses_plain, 0 },
  { "Other",		canon_BJC_7000_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_7000);

/* ----------------------------------- Canon BJC 7100 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_7100_modeuses_plain[] = {
  "1200x1200dpi",
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_7100_modeuses[] = {
  { "Plain",		canon_BJC_7100_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_7100_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_7100_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_7100_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_7100_modeuses_plain, 0 },
  { "Coated",		canon_BJC_7100_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_7100_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_7100_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_7100_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_7100_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_7100_modeuses_plain, 0 },
  { "Other",		canon_BJC_7100_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_7100);

/* ----------------------------------- Canon BJC 8200 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_8200_modeuses_plain[] = {
  "1200x1200dpi",
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_8200_modeuses[] = {
  { "Plain",		canon_BJC_8200_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_8200_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_8200_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_8200_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_8200_modeuses_plain, 0 },
  { "Coated",		canon_BJC_8200_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_8200_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_8200_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_8200_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_8200_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_8200_modeuses_plain, 0 },
  { "Other",		canon_BJC_8200_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_8200);

/* ----------------------------------- Canon BJC 8500 ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_8500_modeuses_plain[] = {
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const canon_modeuse_t canon_BJC_8500_modeuses[] = {
  { "Plain",		canon_BJC_8500_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_8500_modeuses_plain, 0 }, 
  { "BackPrint",	canon_BJC_8500_modeuses_plain, 0 },
  { "Fabric",		canon_BJC_8500_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_8500_modeuses_plain, 0 },
  { "Coated",		canon_BJC_8500_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_8500_modeuses_plain, 0 },
  { "GlossyFilm",	canon_BJC_8500_modeuses_plain, 0 },
  { "GlossyPaper",	canon_BJC_8500_modeuses_plain, 0 },
  { "GlossyCard",	canon_BJC_8500_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_8500_modeuses_plain, 0 },
  { "Other",		canon_BJC_8500_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_8500);

/* ----------------------------------- Canon DS700  ----------------------------------- */
static const char* canon_SELPHY_DS700_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS700_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_SELPHY_DS700_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS700_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS700_modeuses_Hagaki[] = {
  "600x600dpi_photo3",
  "600x600dpi_photodraft2",
  NULL
};

static const canon_modeuse_t canon_SELPHY_DS700_modeuses[] = {
  { "GlossyPro",	canon_SELPHY_DS700_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_SELPHY_DS700_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_SELPHY_DS700_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_SELPHY_DS700_modeuses_PPgloss, 0 },
  { "InkJetHagaki", 	canon_SELPHY_DS700_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_SELPHY_DS700_modeuses_Hagaki, 0 },
};

DECLARE_MODEUSES(canon_SELPHY_DS700);

/* ----------------------------------- Canon DS810  ----------------------------------- */
static const char* canon_SELPHY_DS810_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS810_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_SELPHY_DS810_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS810_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS810_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_SELPHY_DS810_modeuses_Hagaki[] = {
  "600x600dpi_photo3",
  "600x600dpi_photodraft2",
  NULL
};

static const canon_modeuse_t canon_SELPHY_DS810_modeuses[] = {
  { "GlossyPro",	canon_SELPHY_DS810_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_SELPHY_DS810_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_SELPHY_DS810_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_SELPHY_DS810_modeuses_PPgloss, 0 },
  { "InkJetHagaki", 	canon_SELPHY_DS810_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_SELPHY_DS810_modeuses_Hagaki, 0 },
};

DECLARE_MODEUSES(canon_SELPHY_DS810);

/* ----------------------------------- Canon mini220  ----------------------------------- */
static const char* canon_PIXMA_mini220_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh",
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_mini220_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_mini220_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_mini220_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_mini220_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_mini220_modeuses_Hagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_mini220_modeuses[] = {
  { "GlossyPro",	canon_PIXMA_mini220_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_mini220_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_PIXMA_mini220_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_mini220_modeuses_PPgloss, 0 },
  { "InkJetHagaki", 	canon_PIXMA_mini220_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_mini220_modeuses_Hagaki, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_mini220);

/* ----------------------------------- Canon mini320  ----------------------------------- */
/*most nodes not supported*/
static const char* canon_PIXMA_mini320_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh",
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

/*most nodes not supported*/
static const char* canon_PIXMA_mini320_modeuses_PPplus[] = {
  "600x600dpi_photo",
  NULL
};

/*unsupported*/
static const char* canon_PIXMA_mini320_modeuses_PPmatte[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

/*most nodes not supported*/
static const char* canon_PIXMA_mini320_modeuses_PPgloss[] = {
  "600x600dpi_photo",
  NULL
};

/*unsupported*/
static const char* canon_PIXMA_mini320_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

/*unsupported*/
static const char* canon_PIXMA_mini320_modeuses_Hagaki[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

static const canon_modeuse_t canon_PIXMA_mini320_modeuses[] = {
  { "GlossyPro",	canon_PIXMA_mini320_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_mini320_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_mini320_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_PIXMA_mini320_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_mini320_modeuses_PPgloss, 0 },
  { "PhotoPlusGloss2",	canon_PIXMA_mini320_modeuses_PPplus, 0 },
  { "InkJetHagaki", 	canon_PIXMA_mini320_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_mini320_modeuses_Hagaki, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_mini320);

/* ----------------------------------- Canon S200  ----------------------------------- */
/* TODO: mode-media correlation */
static const char* canon_BJC_S200_modeuses_plain[] = {
  "1440x1440dpi",
  "1440x720dpi",
  "720x720dpi",
  "360x360dpi",
  NULL
  };

static const canon_modeuse_t canon_BJC_S200_modeuses[] = {
  { "Plain",            canon_BJC_S200_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S200_modeuses_plain, 0 },
  { "PhotopaperPlus",	canon_BJC_S200_modeuses_plain, 0 },/*untested*/
  { "PhotopaperPlusDouble",canon_BJC_S200_modeuses_plain, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S200_modeuses_plain, 0 },/*untested*/
  { "GlossyPaper",	canon_BJC_S200_modeuses_plain, 0 },
  { "Coated",		canon_BJC_S200_modeuses_plain, 0 },
  { "InkJetHagaki", 	canon_BJC_S200_modeuses_plain, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S200_modeuses_plain, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S200_modeuses_plain, 0 },
  { "TShirt",		canon_BJC_S200_modeuses_plain, 0 },
  { "Transparency",	canon_BJC_S200_modeuses_plain, 0 },
  { "Envelope",		canon_BJC_S200_modeuses_plain, 0 },
  { "PhotopaperOther",	canon_BJC_S200_modeuses_plain, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S200);

/* ----------------------------------- Canon BJC S300 ----------------------------------- */
static const char* canon_BJC_S300_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_std2",
  "600x600dpi",/*untested*/
  "600x00dpi_std4", /* legacy */
  "300x300dpi",
  "300x300dpi_std2", /* legacy */
  NULL
  };

static const char* canon_BJC_S300_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S300_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S300_modeuses_coated[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S300_modeuses_Envelope[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  "600x600dpi_std4", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_S300_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_S300_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const canon_modeuse_t canon_BJC_S300_modeuses[] = {
  { "Plain",            canon_BJC_S300_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S300_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S300_modeuses_PPgloss, 0 },/*untested*/
  { "PhotopaperPlusDouble", canon_BJC_S300_modeuses_PPgloss, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S300_modeuses_PPgloss, 0 },/*untested*/
  { "GlossyPaper",	canon_BJC_S300_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S300_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_S300_modeuses_PPgloss, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S300_modeuses_Envelope, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S300_modeuses_coated, 0 },
  { "GlossyCard", 	canon_BJC_S300_modeuses_PPgloss, 0 },
  { "Transparency", 	canon_BJC_S300_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S300_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S300_modeuses_Envelope, 0 },
  { "PhotopaperOther",	canon_BJC_S300_modeuses_PPgloss, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S300);

/* ----------------------------------- Canon BJC S330 ----------------------------------- */
static const char* canon_BJC_S330_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_std2",
  "600x600dpi",/*untested*/
  "300x300dpi",
  NULL
  };

static const char* canon_BJC_S330_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S330_modeuses_PPmatte[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S330_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S330_modeuses_Envelope[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_BJC_S330_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_S330_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const canon_modeuse_t canon_BJC_S330_modeuses[] = {
  { "Plain",            canon_BJC_S330_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S330_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S330_modeuses_PPpro, 0 },
  { "PhotopaperPlusDouble", canon_BJC_S330_modeuses_PPpro, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S330_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_S330_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S330_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_BJC_S330_modeuses_PPpro, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S330_modeuses_Envelope, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S330_modeuses_PPmatte, 0 },
  { "Transparency", 	canon_BJC_S330_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S330_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S330_modeuses_Envelope, 0 },
  { "PhotopaperOther",	canon_BJC_S330_modeuses_PPpro, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S330);

/* ----------------------------------- Canon BJC S500 ----------------------------------- */
static const char* canon_BJC_S500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",/*untested*/
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_S500_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S500_modeuses_coated[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_S500_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S500_modeuses_Envelope[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_BJC_S500_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_S500_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const canon_modeuse_t canon_BJC_S500_modeuses[] = {
  { "Plain",            canon_BJC_S500_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S500_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S500_modeuses_PPpro, 0 },/*untested*/
  { "PhotopaperPlusDouble", canon_BJC_S500_modeuses_PPpro, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S500_modeuses_coated, 0 },/*untested*/
  { "GlossyPaper",	canon_BJC_S500_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S500_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_S500_modeuses_PPpro, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S500_modeuses_Envelope, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S500_modeuses_PPpro, 0 },
  { "GlossyCard", 	canon_BJC_S500_modeuses_PPgloss, 0 },
  { "Transparency", 	canon_BJC_S500_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S500_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S500_modeuses_Envelope, 0 },
  { "PhotopaperOther",	canon_BJC_S500_modeuses_PPpro, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S500);

/* ----------------------------------- Canon BJC S520 ----------------------------------- */
static const char* canon_BJC_S520_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_highmono",/* mono */
  "600x600dpi",/*untested*/
  "600x600dpi_draft",
  "300x300dpi",/*untested*/
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_S520_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S520_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S520_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S520_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_S520_modeuses_Envelope[] = {
  "600x600dpi_high3",
  "600x600dpi_highmono3",/* mono */
  "600x600dpi_std3",
  NULL
};

static const char* canon_BJC_S520_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_S520_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const canon_modeuse_t canon_BJC_S520_modeuses[] = {
  { "Plain",            canon_BJC_S520_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S520_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S520_modeuses_PPpro, 0 },
  { "PhotopaperPlusDouble", canon_BJC_S520_modeuses_PPpro, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S520_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_S520_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S520_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_S520_modeuses_PPpro, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S520_modeuses_Envelope, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S520_modeuses_PPpro, 0 },
  { "Transparency", 	canon_BJC_S520_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S520_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S520_modeuses_Envelope, 0 },
  { "PhotopaperOther",	canon_BJC_S520_modeuses_PPpro, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S520);

/* ----------------------------------- Canon BJC S600 ----------------------------------- */
static const char* canon_BJC_S600_modeuses_plain[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_std2", /* legacy */
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  "300x300dpi_std2", /* legacy */
  NULL
  };

static const char* canon_BJC_S600_modeuses_PPpro[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_S600_modeuses_coated[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std", /* legacy */
  NULL
};

static const char* canon_BJC_S600_modeuses_PPgloss[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_S600_modeuses_inkjetHagaki[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_S600_modeuses_Envelope[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_S600_modeuses_TShirt[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_tshirt",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_S600_modeuses_Transparency[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh5",
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const canon_modeuse_t canon_BJC_S600_modeuses[] = {
  { "Plain",            canon_BJC_S600_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S600_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S600_modeuses_PPpro, 0 },/*untested*/
  { "PhotopaperPlusDouble", canon_BJC_S600_modeuses_PPpro, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S600_modeuses_PPgloss, 0 },/*untested*/
  { "GlossyPaper",	canon_BJC_S600_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S600_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_S600_modeuses_inkjetHagaki, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S600_modeuses_Envelope, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S600_modeuses_PPpro, 0 },
  { "GlossyCard", 	canon_BJC_S600_modeuses_PPgloss, 0 },
  { "Transparency", 	canon_BJC_S600_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S600_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S600_modeuses_Envelope, 0 },
  { "PhotopaperOther",	canon_BJC_S600_modeuses_PPpro, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S600);

/* ----------------------------------- Canon BJC S750 ----------------------------------- */
static const char* canon_BJC_S750_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",/*untested*/
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_S750_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S750_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S750_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_S750_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_S750_modeuses_Envelope[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_BJC_S750_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_S750_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const canon_modeuse_t canon_BJC_S750_modeuses[] = {
  { "Plain",            canon_BJC_S750_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S750_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S750_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_BJC_S750_modeuses_PPplus, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S750_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_BJC_S750_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S750_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_S750_modeuses_PPplus, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S750_modeuses_Envelope, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S750_modeuses_PPpro, 0 },
  { "Transparency", 	canon_BJC_S750_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S750_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S750_modeuses_Envelope, 0 },
  { "PhotopaperOther",	canon_BJC_S750_modeuses_PPpro, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S750);

/* ----------------------------------- Canon BJC S800 ----------------------------------- */
static const char* canon_BJC_S800_modeuses_plain[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_high2",
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_std3", /* legacy */
  "600x600dpi_draft",
  "300x300dpi", /* legacy */
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  NULL
};

static const char* canon_BJC_S800_modeuses_PPpro[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh4",
  "600x600dpi_photohigh",
  "600x600dpi_std3", /* legacy */
  "300x300dpi", /* legacy */
  NULL
};

static const char* canon_BJC_S800_modeuses_PPplus[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh2",
  "600x600dpi_photo", /*untested*/
  "600x600dpi_std3", /* legacy */
  "300x300dpi", /* legacy */
  NULL
};

static const char* canon_BJC_S800_modeuses_PPgloss[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_std3", /* legacy */
  "300x300dpi", /* legacy */
  NULL
};

static const char* canon_BJC_S800_modeuses_glossFilm[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh2",
  "600x600dpi_photo", /*untested*/
  "600x600dpi_std3", /* legacy */
  "300x300dpi", /* legacy */
  NULL
};

static const char* canon_BJC_S800_modeuses_PPother[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_photohigh4",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  "600x600dpi_std3", /* legacy */
  "300x300dpi", /* legacy */
  /* Mono */
  "600x600dpi_photomonohigh2",
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  "600x600dpi_photomonodraft",
  NULL
};

static const char* canon_BJC_S800_modeuses_TShirt[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_tshirt",
  "600x600dpi_std3", /* legacy */
  "300x300dpi", /* legacy */
  NULL
};

static const char* canon_BJC_S800_modeuses_Transparency[] = {
  "1200x1200dpi", /* legacy */
  "600x600dpi_std2",
  "600x600dpi_draft2",
  "600x600dpi_std3", /* legacy */
  "300x300dpi", /* legacy */
  NULL
};

static const canon_modeuse_t canon_BJC_S800_modeuses[] = {
  { "Plain",            canon_BJC_S800_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S800_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S800_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_BJC_S800_modeuses_PPplus, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S800_modeuses_PPgloss, 0 },
  { "GlossyPaper",	canon_BJC_S800_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S800_modeuses_PPgloss, 0 },
  { "InkJetHagaki", 	canon_BJC_S800_modeuses_PPgloss, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S800_modeuses_plain, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S800_modeuses_glossFilm, 0 },
  { "GlossyCard", 	canon_BJC_S800_modeuses_PPgloss, 0 },
  { "Transparency", 	canon_BJC_S800_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S800_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S800_modeuses_plain, 0 },
  { "PhotopaperOther",	canon_BJC_S800_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_BJC_S800);

/* ----------------------------------- Canon BJC S820 ----------------------------------- */
static const char* canon_BJC_S820_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high", /*untested*/
  "600x600dpi",
  "600x600dpi_draft",
  /* Mono */
  "600x600dpi_highmono2",/*untested*/
  "600x600dpi_highmono", /*untested*/
  "600x600dpi_mono", /*untested*/
  "600x600dpi_draftmono",
  NULL
};

static const char* canon_BJC_S820_modeuses_PPpro[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_BJC_S820_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo", /*untested*/
  NULL
};

static const char* canon_BJC_S820_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S820_modeuses_glossFilm[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo", /*untested*/
  NULL
};

static const char* canon_BJC_S820_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_S820_modeuses_Transparency[] = {
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const canon_modeuse_t canon_BJC_S820_modeuses[] = {
  { "Plain",            canon_BJC_S820_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S820_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S820_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_BJC_S820_modeuses_PPplus, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S820_modeuses_PPgloss, 0 },
  { "GlossyPaper",	canon_BJC_S820_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S820_modeuses_PPgloss, 0 },
  { "InkJetHagaki", 	canon_BJC_S820_modeuses_PPgloss, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S820_modeuses_plain, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S820_modeuses_glossFilm, 0 },
  { "GlossyCard", 	canon_BJC_S820_modeuses_PPgloss, 0 },
  { "Transparency", 	canon_BJC_S820_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S820_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S820_modeuses_plain, 0 },
  { "PhotopaperOther",	canon_BJC_S820_modeuses_plain, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_S820);

/* ----------------------------------- Canon BJC S900 ----------------------------------- */
static const char* canon_BJC_S900_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  /* Mono */
  "600x600dpi_highmono2",/*untested*/
  "600x600dpi_highmono", /*untested*/
  "600x600dpi_mono", /*untested*/
  "600x600dpi_draftmono",
  NULL
};

static const char* canon_BJC_S900_modeuses_PPpro[] = {
  "600x600dpi_photohigh5",
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_BJC_S900_modeuses_PPplus[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo2", /*untested*/
  NULL
};

static const char* canon_BJC_S900_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S900_modeuses_coated[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_S900_modeuses_glossFilm[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo", /*untested*/
  NULL
};

static const char* canon_BJC_S900_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_S900_modeuses_Transparency[] = {
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const canon_modeuse_t canon_BJC_S900_modeuses[] = {
  { "Plain",            canon_BJC_S900_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_S900_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_S900_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_BJC_S900_modeuses_PPplus, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_S900_modeuses_PPgloss, 0 },
  { "GlossyPaper",	canon_BJC_S900_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_S900_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_S900_modeuses_PPgloss, 0 },/*untested*/
  { "Hagaki", 	        canon_BJC_S900_modeuses_plain, 0 },/*untested*/
  { "GlossyFilm", 	canon_BJC_S900_modeuses_glossFilm, 0 },
  { "GlossyCard", 	canon_BJC_S900_modeuses_PPgloss, 0 },
  { "Transparency", 	canon_BJC_S900_modeuses_Transparency, 0 },
  { "TShirt",		canon_BJC_S900_modeuses_TShirt, 0 },
  { "Envelope",		canon_BJC_S900_modeuses_plain, 0 },
  { "PhotopaperOther",	canon_BJC_S900_modeuses_plain, 0 },
};

DECLARE_MODEUSES(canon_BJC_S900);

/* ----------------------------------- Canon i50  ----------------------------------- */
static const char* canon_BJC_i50_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_i50_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i50_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i50_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i50_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photomed",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i50_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",/*color*/
  "600x600dpi_std4",/*mono*/
  "600x600dpi_draft3",
  NULL
};

static const char* canon_BJC_i50_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i50_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_BJC_i50_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
  };

static const canon_modeuse_t canon_BJC_i50_modeuses[] = {
  { "Plain",            canon_BJC_i50_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i50_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i50_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i50_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_BJC_i50_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i50_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i50_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_BJC_i50_modeuses_PPplus, 0 },
  { "Hagaki", 	        canon_BJC_i50_modeuses_Hagaki, 0 },
  { "TShirt",		canon_BJC_i50_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i50_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i50_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i50_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_BJC_i50);

/* ----------------------------------- Canon i80  ----------------------------------- */
static const char* canon_BJC_i80_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/*mono*/
  "600x600dpi",
  "600x600dpi_std2", /* legacy */
  "300x300dpi",
  "300x300dpi_draft",
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_i80_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i80_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i80_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i80_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i80_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_BJC_i80_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_high4",/*mono*/
  "600x600dpi_std3",
  "600x600dpi_draft3",
  "600x600dpi_std2", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_i80_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i80_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  "600x600dpi_ohpdraft",
  NULL
};

static const char* canon_BJC_i80_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_BJC_i80_modeuses[] = {
  { "Plain",            canon_BJC_i80_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i80_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i80_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i80_modeuses_PPmatte, 0 },
  { "PhotopaperMatte",	canon_BJC_i80_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i80_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i80_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_BJC_i80_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i80_modeuses_Hagaki, 0 },
  { "TShirt",		canon_BJC_i80_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i80_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i80_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i80_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_BJC_i80);

/* ----------------------------------- Canon i450  ----------------------------------- */
static const char* canon_BJC_i450_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_highmono",/* mono mode */
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_i450_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i450_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i450_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i450_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i450_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_BJC_i450_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_highmono2",/* mono */
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_BJC_i450_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i450_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_BJC_i450_modeuses_PPother[] = {
  "600x600dpi_photohigh2",/*untested*/
  "600x600dpi_photo2",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_BJC_i450_modeuses[] = {
  { "Plain",            canon_BJC_i450_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i450_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i450_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i450_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_BJC_i450_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_BJC_i450_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i450_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_BJC_i450_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i450_modeuses_Hagaki, 0 },
  { "TShirt",		canon_BJC_i450_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i450_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i450_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i450_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i450);

/* ----------------------------------- Canon i455  ----------------------------------- */
static const char* canon_BJC_i455_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_highmono",/* mono mode */
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_i455_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i455_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i455_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i455_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i455_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_BJC_i455_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_highmono2",/* mono */
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_BJC_i455_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i455_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_BJC_i455_modeuses_PPother[] = {
  "600x600dpi_photohigh2",/*untested*/
  "600x600dpi_photo2",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_BJC_i455_modeuses[] = {
  { "Plain",            canon_BJC_i455_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i455_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i455_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i455_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_BJC_i455_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_BJC_i455_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i455_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_BJC_i455_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i455_modeuses_Hagaki, 0 },
  { "TShirt",		canon_BJC_i455_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i455_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i455_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i455_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i455);

/* ----------------------------------- Canon i550  ----------------------------------- */
static const char* canon_BJC_i550_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_highmono",/* mono mode */
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_i550_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i550_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i550_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photomed",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i550_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i550_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo3",
  "600x600dpi_photo",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i550_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_highmono3",/* mono */
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_BJC_i550_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i550_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

/*untested*/
static const char* canon_BJC_i550_modeuses_PPother[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
  };

static const canon_modeuse_t canon_BJC_i550_modeuses[] = {
  { "Plain",            canon_BJC_i550_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i550_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i550_modeuses_PPpro, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i550_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_BJC_i550_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i550_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i550_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i550_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i550_modeuses_Hagaki, 0 },
  { "GlossyFilm", 	canon_BJC_i550_modeuses_PPpro, 0 },
  { "TShirt",		canon_BJC_i550_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i550_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i550_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i550_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i550);

/* ----------------------------------- Canon i560  ----------------------------------- */
static const char* canon_BJC_i560_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_high3", /* legacy */
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  "300x300dpi_std2", /* legacy */
  NULL
  };

/* highest mode not supported yet */
static const char* canon_BJC_i560_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i560_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i560_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i560_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft2",/*untested*/
  NULL
};

static const char* canon_BJC_i560_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i560_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_high3", /* legacy */
  "300x300dpi_std2", /* legacy */
  NULL
};

static const char* canon_BJC_i560_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_BJC_i560_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i560_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_BJC_i560_modeuses_PPother[] = {
  "600x600dpi_photo",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_BJC_i560_modeuses[] = {
  { "Plain",            canon_BJC_i560_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i560_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i560_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i560_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_BJC_i560_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_BJC_i560_modeuses_PPplusDS, 0 },
  { "Coated",		canon_BJC_i560_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i560_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i560_modeuses_Hagaki, 0 },
  { "DiscCompat", 	canon_BJC_i560_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i560_modeuses_disc, 0 },
  { "TShirt",		canon_BJC_i560_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i560_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i560_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i560_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i560);

/* ----------------------------------- Canon i850  ----------------------------------- */
static const char* canon_BJC_i850_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highset mode not supported yet */
static const char* canon_BJC_i850_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i850_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highset mode not supported yet */
static const char* canon_BJC_i850_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i850_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i850_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i850_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_BJC_i850_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i850_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i850_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

/*untested*/
static const char* canon_BJC_i850_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
  };

static const canon_modeuse_t canon_BJC_i850_modeuses[] = {
  { "Plain",            canon_BJC_i850_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i850_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i850_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i850_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_BJC_i850_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_BJC_i850_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i850_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i850_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i850_modeuses_Hagaki, 0 },
  { "DiscCompat", 	canon_BJC_i850_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i850_modeuses_disc, 0 },
  { "GlossyFilm", 	canon_BJC_i850_modeuses_PPplus, 0 },
  { "TShirt",		canon_BJC_i850_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i850_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i850_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i850_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i850);

/* ----------------------------------- Canon i860  ----------------------------------- */
static const char* canon_BJC_i860_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

/* highest mode not supported yet */
static const char* canon_BJC_i860_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_BJC_i860_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i860_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i860_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_BJC_i860_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_BJC_i860_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i860_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i860_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_BJC_i860_modeuses_PPother[] = {
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_BJC_i860_modeuses[] = {
  { "Plain",            canon_BJC_i860_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_BJC_i860_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i860_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i860_modeuses_PPmatte, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_i860_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i860_modeuses_PPmatte, 0 },
  { "Coated",		canon_BJC_i860_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_BJC_i860_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i860_modeuses_Hagaki, DUPLEX_SUPPORT }, /* not sure */
  { "DiscCompat", 	canon_BJC_i860_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i860_modeuses_disc, 0 },
  { "TShirt",		canon_BJC_i860_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i860_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i860_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i860_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i860);

/* ----------------------------------- Canon i900  ----------------------------------- */
static const char* canon_BJC_i900_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_BJC_i900_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i900_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i900_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i900_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_BJC_i900_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_BJC_i900_modeuses_disc[] = {
  "600x600dpi_photohigh5",
  "600x600dpi_photo5",
  "600x600dpi_photodraft5",
  NULL
};

static const char* canon_BJC_i900_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i900_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_BJC_i900_modeuses_PPother[] = {
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_BJC_i900_modeuses[] = {
  { "Plain",            canon_BJC_i900_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i900_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i900_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i900_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_BJC_i900_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i900_modeuses_PPplus, 0 },
  { "Coated",		canon_BJC_i900_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_BJC_i900_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i900_modeuses_Hagaki, 0 },
  { "DiscCompat", 	canon_BJC_i900_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i900_modeuses_disc, 0 },
  { "TShirt",		canon_BJC_i900_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i900_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i900_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i900_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i900);

/* ----------------------------------- Canon i950  ----------------------------------- */
static const char* canon_BJC_i950_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high", /*untested*/
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "600x600dpi_draft2",
  "600x600dpi_draftmono",/* mono --- untested*/
  "600x600dpi_draftmono2",/* mono */
  NULL
};

static const char* canon_BJC_i950_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_BJC_i950_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2", /*untested*/
  NULL
};

static const char* canon_BJC_i950_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i950_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i950_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo3",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i950_modeuses_GlossyFilm[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_BJC_i950_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_BJC_i950_modeuses_disc[] = {
  "600x600dpi_photo5",
  "600x600dpi_photodraft5",
  NULL
};

static const char* canon_BJC_i950_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_high5", /*untested*/
  "600x600dpi_std4",/*untested*/
  "600x600dpi_std5",
  "600x600dpi_draftmono4",/* mono---untested*/
  "600x600dpi_draftmono5",/* mono */
  NULL
};

static const char* canon_BJC_i950_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i950_modeuses_Transparency[] = {
  "600x600dpi_std3",
  "600x600dpi_draft3",
  "600x600dpi_draft4",
  NULL
};

static const char* canon_BJC_i950_modeuses_PPother[] = {
  "600x600dpi_photo2", /*untested*/
  NULL
  };

static const canon_modeuse_t canon_BJC_i950_modeuses[] = {
  { "Plain",            canon_BJC_i950_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i950_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i950_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i950_modeuses_PPgloss, 0 },
  { "PhotopaperMatte",	canon_BJC_i950_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i950_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i950_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i950_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i950_modeuses_Hagaki, 0 },
  { "DiscCompat", 	canon_BJC_i950_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i950_modeuses_disc, 0 },
  { "GlossyFilm", 	canon_BJC_i950_modeuses_GlossyFilm, 0 },
  { "TShirt",		canon_BJC_i950_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i950_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i950_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i950_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i950);

/* ----------------------------------- Canon i960  ----------------------------------- */
static const char* canon_BJC_i960_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high3", /* duplex */
  "600x600dpi_high",
  "600x600dpi", /*untested*/
  "600x600dpi_draft",
  "600x600dpi_draft2",
  "600x600dpi_draftmono",/* mono */
  "600x600dpi_draftmono2",/* mono */
  NULL
};

static const char* canon_BJC_i960_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i960_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo5", /*untested*/
  NULL
};

static const char* canon_BJC_i960_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i960_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i960_modeuses_coated[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  "600x600dpi_photo5",
  NULL
};

static const char* canon_BJC_i960_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_BJC_i960_modeuses_disc[] = {
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_BJC_i960_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_high5",
  "600x600dpi_std4", /*untested*/
  "600x600dpi_std5",
  "600x600dpi_draft5",
  "600x600dpi_draftmono4",
  "600x600dpi_draftmono5",
  NULL
};

static const char* canon_BJC_i960_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i960_modeuses_Transparency[] = {
  "600x600dpi_std3",
  "600x600dpi_draft3",
  "600x600dpi_draft4", /*untested*/
  NULL
};

static const char* canon_BJC_i960_modeuses_PPother[] = {
  "600x600dpi_photo2", /*untested*/
  NULL
  };

static const canon_modeuse_t canon_BJC_i960_modeuses[] = {
  { "Plain",            canon_BJC_i960_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "GlossyPro",	canon_BJC_i960_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i960_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i960_modeuses_PPgloss, 0 },
  { "PhotopaperMatte",	canon_BJC_i960_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i960_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i960_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i960_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i960_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat", 	canon_BJC_i960_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i960_modeuses_disc, 0 },
  { "TShirt",		canon_BJC_i960_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i960_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i960_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i960_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i960);

/* ----------------------------------- Canon i990 ----------------------------------- */
static const char* canon_BJC_i990_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high3", /*duplex*/
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  "600x600dpi_draftmono",/*mono*/
  "600x600dpi_draftmono2",/*mono*/
  NULL
};

/* most photo modes use R ink and therefore unsupported */
/* unsupported */
static const char* canon_BJC_i990_modeuses_PPpro[] = {
  "600x600dpi_photohigh", /*stand-in*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_BJC_i990_modeuses_PPplus[] = {
  "600x600dpi_photohigh", /*stand-in*/
  "600x600dpi_photodraft",
  NULL
};

/* unsupported */
static const char* canon_BJC_i990_modeuses_PPmatte[] = {
  "600x600dpi_photohigh", /*stand-in*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_BJC_i990_modeuses_PPgloss[] = {
  "600x600dpi_photohigh", /*stand-in*/
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_BJC_i990_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photomed",
  "600x600dpi_photo",
  NULL
};

/* high mode not yet supported */
static const char* canon_BJC_i990_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i990_modeuses_disc[] = {
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i990_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft5",
  "600x600dpi_draft4",
  "600x600dpi_draftmono4",/*mono*/
  "600x600dpi_draftmono5",/*mono*/
  NULL
};

static const char* canon_BJC_i990_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i990_modeuses_Transparency[] = {
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

/* untested */
static const char* canon_BJC_i990_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_BJC_i990_modeuses[] = {
  { "Plain",            canon_BJC_i990_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "GlossyPro",	canon_BJC_i990_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i990_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i990_modeuses_PPplus, 0 },/*untested*/
  { "PhotopaperMatte",	canon_BJC_i990_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i990_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i990_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i990_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i990_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat", 	canon_BJC_i990_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i990_modeuses_disc, 0 },
  { "TShirt",		canon_BJC_i990_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i990_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i990_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i990_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i990);

/* ----------------------------------- Canon i6100  ----------------------------------- */
static const char* canon_BJC_i6100_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",/*untested*/
  "300x300dpi_draft",
  NULL
  };

static const char* canon_BJC_i6100_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i6100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i6100_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i6100_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i6100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i6100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_BJC_i6100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i6100_modeuses_Transparency[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_BJC_i6100_modeuses_PPother[] = {
  "600x600dpi_photo2",/*untested*/
  "600x600dpi_photo",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_BJC_i6100_modeuses[] = {
  { "Plain",            canon_BJC_i6100_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i6100_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i6100_modeuses_PPpro, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i6100_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_BJC_i6100_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i6100_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i6100_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i6100_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i6100_modeuses_Hagaki, 0 },
  { "TShirt",		canon_BJC_i6100_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i6100_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i6100_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i6100_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i6100);

/* ----------------------------------- Canon i9100  ----------------------------------- */
static const char* canon_BJC_i9100_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "600x600dpi_mono",/*untested*/
  "600x600dpi_draftmono",
  NULL
};

static const char* canon_BJC_i9100_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i9100_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2", /*untested*/
  NULL
};

static const char* canon_BJC_i9100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i9100_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_BJC_i9100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_BJC_i9100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  "600x600dpi_draftmono2",
  NULL
};

static const char* canon_BJC_i9100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i9100_modeuses_Transparency[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3", /*untested*/
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i9100_modeuses_PPother[] = {
  "600x600dpi_photo2", /*untested*/
  "600x600dpi_photo", /*untested*/
  NULL
};

static const canon_modeuse_t canon_BJC_i9100_modeuses[] = {
  { "Plain",            canon_BJC_i9100_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i9100_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i9100_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i9100_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_BJC_i9100_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i9100_modeuses_PPmatte, 0 },
  { "Coated",		canon_BJC_i9100_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i9100_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i9100_modeuses_Hagaki, 0 },
  { "TShirt",		canon_BJC_i9100_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i9100_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i9100_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i9100_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_BJC_i9100);

/* ----------------------------------- Canon i9900  ----------------------------------- */
static const char* canon_BJC_i9900_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  "600x600dpi_draftmono",/*mono*/
  "600x600dpi_draftmono2",/*mono*/
  NULL
};

/* most photo modes use R,G inks and therefore unsupported */
/* unsupported */
static const char* canon_BJC_i9900_modeuses_PPpro[] = {
  "600x600dpi_photohigh", /*stand-in*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_BJC_i9900_modeuses_PPplus[] = {
  "600x600dpi_photohigh", /*stand-in*/
  "600x600dpi_photodraft",
  NULL
};

/* unsupported */
static const char* canon_BJC_i9900_modeuses_PPmatte[] = {
  "600x600dpi_photohigh", /*stand-in*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_BJC_i9900_modeuses_PPgloss[] = {
  "600x600dpi_photohigh", /*stand-in*/
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_BJC_i9900_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photomed",
  "600x600dpi_photo",
  NULL
};

/* high mode not yet supported */
static const char* canon_BJC_i9900_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  NULL
};

static const char* canon_BJC_i9900_modeuses_disc[] = {
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_BJC_i9900_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft5",
  "600x600dpi_draft4",
  "600x600dpi_draftmono4",/*mono*/
  "600x600dpi_draftmono5",/*mono*/
  NULL
};

static const char* canon_BJC_i9900_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_BJC_i9900_modeuses_Transparency[] = {
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_BJC_i9900_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_BJC_i9900_modeuses[] = {
  { "Plain",            canon_BJC_i9900_modeuses_plain, 0 },
  { "GlossyPro",	canon_BJC_i9900_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_BJC_i9900_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_BJC_i9900_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_BJC_i9900_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_BJC_i9900_modeuses_PPgloss, 0 },
  { "Coated",		canon_BJC_i9900_modeuses_coated, 0 },
  { "InkJetHagaki", 	canon_BJC_i9900_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_BJC_i9900_modeuses_Hagaki, 0 },
  { "DiscCompat", 	canon_BJC_i9900_modeuses_disc, 0 },
  { "DiscOthers", 	canon_BJC_i9900_modeuses_disc, 0 },
  { "TShirt",		canon_BJC_i9900_modeuses_TShirt, 0 },
  { "Transparency",	canon_BJC_i9900_modeuses_Transparency, 0 },
  { "Envelope",		canon_BJC_i9900_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_BJC_i9900_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_BJC_i9900);

/* ----------------------------------- Canon iP90 ----------------------------------- */
static const char* canon_PIXMA_iP90_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high", /* mono */
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_PIXMA_iP90_modeuses_PPpro[] = {
  "600x600dpi_photohigh2", /* no ESC (S */
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP90_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP90_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP90_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_high3", /* mono */
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_PIXMA_iP90_modeuses_TShirt[] = {
  "600x600dpi_tshirt", /* no ESC (S */
  NULL
};

static const char* canon_PIXMA_iP90_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP90_modeuses[] = {
  { "Plain",            canon_PIXMA_iP90_modeuses_plain, 0 },/*INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL*/
  { "GlossyPro",	canon_PIXMA_iP90_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlus",	canon_PIXMA_iP90_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlusDouble", canon_PIXMA_iP90_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_iP90_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_iP90_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_iP90_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_iP90_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_iP90_modeuses_Hagaki, 0 },/*INKSET_COLOR_SUPPORT*/
  { "TShirt",		canon_PIXMA_iP90_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_iP90_modeuses_Hagaki, 0 },/* INKSET_COLOR_SUPPORT*/
  { "PhotopaperOther",	canon_PIXMA_iP90_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_iP90);

/* ----------------------------------- Canon iP100 ----------------------------------- */
static const char* canon_PIXMA_iP100_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high", /* mono */
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_PIXMA_iP100_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP100_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP100_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_high3", /* mono */
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP100_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP100_modeuses[] = {
  { "Plain",            canon_PIXMA_iP100_modeuses_plain, 0 },/*INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL*/
  { "GlossyPro",	canon_PIXMA_iP100_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlus",	canon_PIXMA_iP100_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotoPlusGloss2",	canon_PIXMA_iP100_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlusDouble", canon_PIXMA_iP100_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_iP100_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_iP100_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_iP100_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_iP100_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_iP100_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_iP100_modeuses_Hagaki, 0 },/*INKSET_COLOR_SUPPORT*/
  { "TShirt",		canon_PIXMA_iP100_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_iP100_modeuses_Hagaki, 0 },/*INKSET_COLOR_SUPPORT*/
  { "PhotopaperOther",	canon_PIXMA_iP100_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_iP100);

/* ----------------------------------- Canon iP110 ----------------------------------- */
static const char* canon_PIXMA_iP110_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high", /* mono */
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_PIXMA_iP110_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP110_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP110_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP110_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_high3", /* mono */
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP110_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP110_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP110_modeuses[] = {
  { "Plain",            canon_PIXMA_iP110_modeuses_plain, 0 },/*INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL*/
  { "PhotoPlusGloss2",	canon_PIXMA_iP110_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",	canon_PIXMA_iP110_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",	canon_PIXMA_iP110_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss", canon_PIXMA_iP110_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",canon_PIXMA_iP110_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_iP110_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_iP110_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_iP110_modeuses_Hagaki, 0 },/*INKSET_COLOR_SUPPORT*/
  { "InkJetHagaki", 	canon_PIXMA_iP110_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiK", 	canon_PIXMA_iP110_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_iP110_modeuses_Hagaki, 0 },/*INKSET_COLOR_SUPPORT*/
  { "TShirt",		canon_PIXMA_iP110_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_iP110_modeuses_Hagaki, 0 },/*INKSET_COLOR_SUPPORT*/
  { "PhotopaperOther",	canon_PIXMA_iP110_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_iP110);

/* ----------------------------------- Canon iP2000  ----------------------------------- */
static const char* canon_PIXMA_iP2000_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/* mono */
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_iP2000_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP2000_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP2000_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP2000_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP2000_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_high3",/* mono */
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_PIXMA_iP2000_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP2000_modeuses_Transparency[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_iP2000_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP2000_modeuses[] = {
  { "Plain",            canon_PIXMA_iP2000_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP2000_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP2000_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_PIXMA_iP2000_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP2000_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_PIXMA_iP2000_modeuses_PPgloss, 0 },
  { "Coated",		canon_PIXMA_iP2000_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP2000_modeuses_PPplusDS, 0 },/*untested*/
  { "Hagaki", 	        canon_PIXMA_iP2000_modeuses_Hagaki, 0 },/*untested*/
  { "TShirt",		canon_PIXMA_iP2000_modeuses_TShirt, 0 },
  { "Transparency",	canon_PIXMA_iP2000_modeuses_Transparency, 0 },
  { "Envelope",		canon_PIXMA_iP2000_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP2000_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP2000);

/* ----------------------------------- Canon iP2700  ----------------------------------- */
static const char* canon_PIXMA_iP2700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "300x300dpi_mono",
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "300x300dpi_std3",
  NULL
  };

static const char* canon_PIXMA_iP2700_modeuses_PPpro[] = {
  "1200x1200dpi_high",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP2700_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP2700_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_PIXMA_iP2700_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_iP2700_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP2700_modeuses[] = {
  { "Plain",            canon_PIXMA_iP2700_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",	canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoPro2",	canon_PIXMA_iP2700_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",	canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_iP2700_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_iP2700_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_iP2700_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_iP2700_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_iP2700_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_PIXMA_iP2700_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_iP2700);

/* ----------------------------------- Canon iP3000  ----------------------------------- */
static const char* canon_PIXMA_iP3000_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not supported yet */
static const char* canon_PIXMA_iP3000_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP3000_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP3000_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* Note: iP3000 US driver does not have inkjetHagaki or Hagaki media */
/* untested */
static const char* canon_PIXMA_iP3000_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP3000_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP3000_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP3000_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP3000_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const char* canon_PIXMA_iP3000_modeuses_transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP3000_modeuses[] = {
  { "Plain",            canon_PIXMA_iP3000_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_PIXMA_iP3000_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP3000_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP3000_modeuses_inkjetHagaki, 0 },/*untested*/
  { "Hagaki", 	        canon_PIXMA_iP3000_modeuses_Hagaki, DUPLEX_SUPPORT },/*untested*/
  { "DiscCompat",	canon_PIXMA_iP3000_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP3000_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP3000_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP3000_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP3000_modeuses_PPother, 0 },
  { "Transparency",     canon_PIXMA_iP3000_modeuses_transparency, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP3000);

/* ----------------------------------- Canon iP3100  ----------------------------------- */
static const char* canon_PIXMA_iP3100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not supported yet */
static const char* canon_PIXMA_iP3100_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP3100_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP3100_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP3100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_PIXMA_iP3100_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP3100_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP3100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* Note: iP3100 US driver does not have this media */
/*untested*/
static const char* canon_PIXMA_iP3100_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const char* canon_PIXMA_iP3100_modeuses_transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP3100_modeuses[] = {
  { "Plain",            canon_PIXMA_iP3100_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_PIXMA_iP3100_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP3100_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP3100_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP3100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_iP3100_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP3100_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP3100_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP3100_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP3100_modeuses_PPother, 0 },/*untested*/
  { "Transparency",     canon_PIXMA_iP3100_modeuses_transparency, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP3100);

/* ----------------------------------- Canon iP3300 ----------------------------------- */
static const char* canon_PIXMA_iP3300_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_PIXMA_iP3300_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP3300_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP3300_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP3300_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_PIXMA_iP3300_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_iP3300_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP3300_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP3300_modeuses[] = {
  { "Plain",            canon_PIXMA_iP3300_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP3300_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP3300_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP3300_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP3300_modeuses_PPplusDS, 0 },/*check*/
  { "PhotopaperMatte",	canon_PIXMA_iP3300_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP3300_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP3300_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP3300_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_iP3300_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP3300_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP3300_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP3300);

/* ----------------------------------- Canon iP3600 ----------------------------------- */
static const char* canon_PIXMA_iP3600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_PIXMA_iP3600_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP3600_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP3600_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP3600_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP3600_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP3600_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP3600_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP3600_modeuses[] = {
  { "Plain",            canon_PIXMA_iP3600_modeuses_plain, 0 },
  { "PhotoPlusGloss2",	canon_PIXMA_iP3600_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_PIXMA_iP3600_modeuses_PPpro, 0 },
  { "PhotoProPlat",     canon_PIXMA_iP3600_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss", canon_PIXMA_iP3600_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iP3600_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP3600_modeuses_PPmatte, 0 },
  { "Coated",		canon_PIXMA_iP3600_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP3600_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP3600_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_iP3600_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP3600_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP3600_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP3600);

/* ----------------------------------- Canon iP4000  ----------------------------------- */
static const char* canon_PIXMA_iP4000_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  "600x600dpi_high2",/*legacy*/
  "600x600dpi_std2",/*legacy*/
  "600x600dpi_draft2",/*legacy*/
  NULL
  };

/* highest mode not supported yet */
static const char* canon_PIXMA_iP4000_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP4000_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP4000_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* Note: iP4000 US driver does not have inkjetHagaki or Hagaki media */
/* untested */
static const char* canon_PIXMA_iP4000_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP4000_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_iP4000_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP4000_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP4000_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const char* canon_PIXMA_iP4000_modeuses_transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP4000_modeuses[] = {
  { "Plain",            canon_PIXMA_iP4000_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_PIXMA_iP4000_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP4000_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4000_modeuses_inkjetHagaki, 0 },/*untested*/
  { "Hagaki", 	        canon_PIXMA_iP4000_modeuses_Hagaki, DUPLEX_SUPPORT },/*untested*/
  { "CD",   	        canon_PIXMA_iP4000_modeuses_plain, 0 },/*NOTE:temporary replacement*/
  { "DiscCompat",	canon_PIXMA_iP4000_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP4000_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP4000_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4000_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4000_modeuses_PPother, 0 },
  { "Transparency",     canon_PIXMA_iP4000_modeuses_transparency, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP4000);

/* ----------------------------------- Canon iP4100  ----------------------------------- */
static const char* canon_PIXMA_iP4100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not supported yet */
static const char* canon_PIXMA_iP4100_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP4100_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP4100_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP4100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP4100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_iP4100_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP4100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP4100_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const char* canon_PIXMA_iP4100_modeuses_transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP4100_modeuses[] = {
  { "Plain",            canon_PIXMA_iP4100_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_PIXMA_iP4100_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP4100_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4100_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_iP4100_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP4100_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP4100_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4100_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4100_modeuses_PPother, 0 },/*untested*/
  { "Transparency",     canon_PIXMA_iP4100_modeuses_transparency, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP4100);

/* ----------------------------------- Canon iP4200  ----------------------------------- */
static const char* canon_PIXMA_iP4200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* most photo modes not supported yet --- used photodraft as stand-in everywhere */
static const char* canon_PIXMA_iP4200_modeuses_PPplus[] = {
  "600x600dpi_photodraft",
  NULL
};

/* high mode not yet supported */
static const char* canon_PIXMA_iP4200_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP4200_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP4200_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iP4200_modeuses_transparency[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP4200_modeuses[] = {
  { "Plain",             canon_PIXMA_iP4200_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	        canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperPlus",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* mostly not yet supported */
  { "PhotopaperPlusDouble", canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperMatte",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "GlossyPaper",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "Coated",		canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "InkJetHagaki", 	canon_PIXMA_iP4200_modeuses_inkjetHagaki, 0 },/* partially not yet supported */
  { "Hagaki", 	        canon_PIXMA_iP4200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "CD",   	        canon_PIXMA_iP4200_modeuses_plain, 0 },/*NOTE:temporary replacement*/
  { "DiscCompat",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "DiscOthers",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "TShirt",		canon_PIXMA_iP4200_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4200_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "Transparency",      canon_PIXMA_iP4200_modeuses_transparency, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_PIXMA_iP4200);

/* ----------------------------------- Canon iP4300  ----------------------------------- */
static const char* canon_PIXMA_iP4300_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* most photo modes not supported yet --- used photodraft as stand-in everywhere */
static const char* canon_PIXMA_iP4300_modeuses_PPplus[] = {
  "600x600dpi_photodraft",
  NULL
};

/* high mode not yet supported */
static const char* canon_PIXMA_iP4300_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP4300_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP4300_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iP4300_modeuses_transparency[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP4300_modeuses[] = {
  { "Plain",             canon_PIXMA_iP4300_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	        canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperPlus",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* mostly not yet supported */
  { "PhotopaperPlusDouble", canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperMatte",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "GlossyPaper",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "Coated",		canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "InkJetHagaki", 	canon_PIXMA_iP4300_modeuses_inkjetHagaki, 0 },/* partially not yet supported */
  { "Hagaki", 	        canon_PIXMA_iP4300_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "CD",   	        canon_PIXMA_iP4300_modeuses_plain, 0 },/*NOTE:temporary replacement*/
  { "DiscCompat",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "DiscOthers",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "TShirt",		canon_PIXMA_iP4300_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4300_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "Transparency",      canon_PIXMA_iP4300_modeuses_transparency, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_PIXMA_iP4300);

/* ----------------------------------- Canon iP4500  ----------------------------------- */
static const char* canon_PIXMA_iP4500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_iP4500_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP4500_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_PIXMA_iP4500_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP4500_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP4500_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_PIXMA_iP4500_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP4500_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_PIXMA_iP4500_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP4500_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP4500_modeuses[] = {
  { "Plain",            canon_PIXMA_iP4500_modeuses_plain, DUPLEX_SUPPORT },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4500_modeuses_PPplus, 0 },
  { "GlossyPro",	canon_PIXMA_iP4500_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP4500_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP4500_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4500_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4500_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4500_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4500_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4500_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_iP4500_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP4500_modeuses_disc, 0 },
  { "TShirt",	        canon_PIXMA_iP4500_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4500_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4500_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP4500);

/* ----------------------------------- Canon iP4600  ----------------------------------- */
static const char* canon_PIXMA_iP4600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_iP4600_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",/*untested*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_PIXMA_iP4600_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",/*untested*/
  NULL
};

static const char* canon_PIXMA_iP4600_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_PIXMA_iP4600_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",/*untested*/
  NULL
};

static const char* canon_PIXMA_iP4600_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  NULL
};

static const char* canon_PIXMA_iP4600_modeuses_disc[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_PIXMA_iP4600_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP4600_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP4600_modeuses[] = {
  { "Plain",            canon_PIXMA_iP4600_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_PIXMA_iP4600_modeuses_PPpro, 0 },
  { "PhotoProPlat",	canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4600_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4600_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4600_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4600_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_iP4600_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP4600_modeuses_disc, 0 },
  { "TShirt",	        canon_PIXMA_iP4600_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4600_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4600_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP4600);

/* ----------------------------------- Canon iP4700  ----------------------------------- */
static const char* canon_PIXMA_iP4700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_iP4700_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_PIXMA_iP4700_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP4700_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP4700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP4700_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_iP4700_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_iP4700_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP4700_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP4700_modeuses[] = {
  { "Plain",            canon_PIXMA_iP4700_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_PIXMA_iP4700_modeuses_PPpro, 0 },
  { "PhotoProPlat",	canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4700_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4700_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_iP4700_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_iP4700_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_iP4700_modeuses_PPpro, 0 },
  { "OtherPhotoHagakiO",canon_PIXMA_iP4700_modeuses_PPpro, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4700_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_iP4700_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP4700_modeuses_disc, 0 },
  { "TShirt",	        canon_PIXMA_iP4700_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4700_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4700_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP4700);

/* ----------------------------------- Canon iP4900  ----------------------------------- */
static const char* canon_PIXMA_iP4900_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_iP4900_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_PIXMA_iP4900_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP4900_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP4900_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_iP4900_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP4900_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP4900_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP4900_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP4900_modeuses[] = {
  { "Plain",            canon_PIXMA_iP4900_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",	canon_PIXMA_iP4900_modeuses_PPproPlat, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4900_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4900_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_iP4900_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_iP4900_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4900_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_iP4900_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP4900_modeuses_disc, 0 },
  { "TShirt",	        canon_PIXMA_iP4900_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP4900_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP4900_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP4900);

/* ----------------------------------- Canon iP5000  ----------------------------------- */
/* high mode not supported yet */
static const char* canon_PIXMA_iP5000_modeuses_plain[] = {
  "600x600dpi",
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* high modes not supported yet */
static const char* canon_PIXMA_iP5000_modeuses_PPpro[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
};

/* high modes not supported yet */
static const char* canon_PIXMA_iP5000_modeuses_PPplus[] = {
  "600x600dpi_photodraft",
  NULL
};

/* high modes not supported yet */
static const char* canon_PIXMA_iP5000_modeuses_PPplusDS[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
};

/* US driver does not supply this media: untested */
static const char* canon_PIXMA_iP5000_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

/* high modes not supported yet */
static const char* canon_PIXMA_iP5000_modeuses_disc[] = {
  /* plain mode temporarily added here */
  "600x600dpi",
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* photo modes temporarily added */
  "600x600dpi_photodraft",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_iP5000_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* high modes not supported yet */
static const char* canon_PIXMA_iP5000_modeuses_PPother[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
  };

static const char* canon_PIXMA_iP5000_modeuses_transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP5000_modeuses[] = {
  { "Plain",            canon_PIXMA_iP5000_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_PIXMA_iP5000_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP5000_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },/*untested*/
  { "Hagaki", 	        canon_PIXMA_iP5000_modeuses_Hagaki, DUPLEX_SUPPORT },/*untested*/
  { "DiscCompat",	canon_PIXMA_iP5000_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP5000_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP5000_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP5000_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP5000_modeuses_PPother, 0 },
  { "Transparency",     canon_PIXMA_iP5000_modeuses_transparency, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP5000);

/* ----------------------------------- Canon iP5300  ----------------------------------- */
static const char* canon_PIXMA_iP5300_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_PIXMA_iP5300_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP5300_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP5300_modeuses[] = {
  { "Plain",            canon_PIXMA_iP5300_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_PIXMA_iP5300_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP5300_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP5300_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP5300_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP5300_modeuses_PPmatte, 0 },
  { "Coated",		canon_PIXMA_iP5300_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP5300_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP5300_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "CD",   	        canon_PIXMA_iP5300_modeuses_plain, 0 },/*optional using plain*/
  { "DiscCompat",	canon_PIXMA_iP5300_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP5300_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP5300_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP5300_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP5300_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP5300);

/* ----------------------------------- Canon iP6000  ----------------------------------- */
static const char* canon_PIXMA_iP6000_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_iP6000_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6000_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6000_modeuses_PPmatte[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo",
  NULL
};

/*Note: US printer driver does not support inkjetHagaki */
static const char* canon_PIXMA_iP6000_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/*Note: US printer driver does not support Hagaki */
static const char* canon_PIXMA_iP6000_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_PIXMA_iP6000_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6000_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP6000_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const char* canon_PIXMA_iP6000_modeuses_transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP6000_modeuses[] = {
 { "Plain",             canon_PIXMA_iP6000_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP6000_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP6000_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP6000_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP6000_modeuses_inkjetHagaki, 0 },/*untested*/
 { "Hagaki", 	        canon_PIXMA_iP6000_modeuses_Hagaki, DUPLEX_SUPPORT },/* untested */
 { "CD",   	        canon_PIXMA_iP6000_modeuses_disc, 0 },/*NOTE:temporary*/
 { "DiscCompat",	canon_PIXMA_iP6000_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP6000_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP6000_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP6000_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP6000_modeuses_PPother, 0 },
 { "Transparency",      canon_PIXMA_iP6000_modeuses_transparency, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP6000);

/* ----------------------------------- Canon iP6100  ----------------------------------- */
static const char* canon_PIXMA_iP6100_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_iP6100_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6100_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",/*untested*/
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_PIXMA_iP6100_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP6100_modeuses_PPother[] = {
  "600x600dpi_photohigh",/*untested*/
  NULL
  };

static const char* canon_PIXMA_iP6100_modeuses_transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP6100_modeuses[] = {
  { "Plain",            canon_PIXMA_iP6100_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "GlossyPro",	canon_PIXMA_iP6100_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP6100_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP6100_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP6100_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_iP6100_modeuses_PPplus, 0 },
  { "Coated",		canon_PIXMA_iP6100_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP6100_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP6100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "CD",   	        canon_PIXMA_iP6100_modeuses_disc, 0 },/*NOTE:identical, so combined*/
  { "TShirt",		canon_PIXMA_iP6100_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP6100_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP6100_modeuses_PPother, 0 },
  { "Transparency",     canon_PIXMA_iP6100_modeuses_transparency, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP6100);

/* ----------------------------------- Canon iP6210  ----------------------------------- */
static const char* canon_PIXMA_iP6210_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "300x300dpi_std2",
  "300x300dpi_draft2",
  NULL
  };

static const char* canon_PIXMA_iP6210_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* Color */
  "1200x1200dpi_photohigh4",
  "600x600dpi_photohigh3",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP6210_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* Color */
  "600x600dpi_photohigh5",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP6210_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* Color */
  "600x600dpi_photohigh5",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP6210_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",
  NULL
};

static const char* canon_PIXMA_iP6210_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  /* Color */
  "600x600dpi_std5",
  NULL
};

static const char* canon_PIXMA_iP6210_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  /* Color */
  "600x600dpi_photohigh3",
  "600x600dpi_photo2",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP6210_modeuses[] = {
  { "Plain",            canon_PIXMA_iP6210_modeuses_plain, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "GlossyPro",	canon_PIXMA_iP6210_modeuses_PPpro, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "PhotopaperPlus",	canon_PIXMA_iP6210_modeuses_PPplus, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "PhotopaperPlusDouble", canon_PIXMA_iP6210_modeuses_PPplusDS, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "PhotopaperMatte",	canon_PIXMA_iP6210_modeuses_PPplusDS, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "GlossyPaper",	canon_PIXMA_iP6210_modeuses_PPplusDS, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "Coated",		canon_PIXMA_iP6210_modeuses_PPplusDS, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_iP6210_modeuses_PPplusDS, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "Hagaki", 	        canon_PIXMA_iP6210_modeuses_Hagaki, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_iP6210_modeuses_TShirt, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "Envelope",		canon_PIXMA_iP6210_modeuses_Hagaki, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_PIXMA_iP6210_modeuses_PPother, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
};

DECLARE_MODEUSES(canon_PIXMA_iP6210);

/* ----------------------------------- Canon iP6600  ----------------------------------- */
static const char* canon_PIXMA_iP6600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_PIXMA_iP6600_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6600_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6600_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP6600_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP6600_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_PIXMA_iP6600_modeuses_disc[] = {
  "600x600dpi_photohigh5",
  "600x600dpi_photo5",
  "600x600dpi_photodraft5",
  NULL
};

static const char* canon_PIXMA_iP6600_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP6600_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP6600_modeuses[] = {
 { "Plain",             canon_PIXMA_iP6600_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_PIXMA_iP6600_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP6600_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP6600_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP6600_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP6600_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP6600_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP6600_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iP6600_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "DiscCompat",	canon_PIXMA_iP6600_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP6600_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP6600_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP6600_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP6600_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP6600);

/* ----------------------------------- Canon iP6700  ----------------------------------- */
static const char* canon_PIXMA_iP6700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_PIXMA_iP6700_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_FA[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_disc[] = {
  "600x600dpi_photohigh5",
  "600x600dpi_photo5",
  "600x600dpi_photodraft5",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP6700_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP6700_modeuses[] = {
 { "Plain",             canon_PIXMA_iP6700_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_PIXMA_iP6700_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP6700_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP6700_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP6700_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP6700_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP6700_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP6700_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iP6700_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "FineArtPhotoRag",   canon_PIXMA_iP6700_modeuses_FA, 0 },
 { "FineArtOther",      canon_PIXMA_iP6700_modeuses_FA, 0 },
 { "DiscCompat",	canon_PIXMA_iP6700_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP6700_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP6700_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP6700_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP6700_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP6700);

/* ----------------------------------- Canon iP7100  ----------------------------------- */
static const char* canon_PIXMA_iP7100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2", /* duplex */
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  /* Mono */
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  NULL
  };

static const char* canon_PIXMA_iP7100_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  /* Mono */
  "600x600dpi_mono",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_FA[] = {
  "600x600dpi_photohigh3",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_disc[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_TShirt[] = {
  "600x600dpi",
  NULL
};

static const char* canon_PIXMA_iP7100_modeuses_Transparency[] = {
  "600x600dpi_std4",
  "600x600dpi_draft4",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iP7100_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP7100_modeuses[] = {
 { "Plain",             canon_PIXMA_iP7100_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP7100_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP7100_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP7100_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP7100_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP7100_modeuses_PPplusDS, 0 },
 { "Coated",		canon_PIXMA_iP7100_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP7100_modeuses_PPplus, 0 },
 { "Hagaki", 	        canon_PIXMA_iP7100_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "FineArtPhotoRag", 	canon_PIXMA_iP7100_modeuses_FA, 0 },
 { "FineArtOther", 	canon_PIXMA_iP7100_modeuses_FA, 0 },
 { "DiscCompat",	canon_PIXMA_iP7100_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP7100_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP7100_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP7100_modeuses_Hagaki, 0 },
 { "Transparency",	canon_PIXMA_iP7100_modeuses_Transparency, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP7100_modeuses_PPother, 0 }, /*untested*/
 };

DECLARE_MODEUSES(canon_PIXMA_iP7100);

/* ----------------------------------- Canon iP7200  ----------------------------------- */
static const char* canon_PIXMA_iP7200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/*duplex*/
  "300x300dpi",
  NULL
};

static const char* canon_PIXMA_iP7200_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode still unsupported */
static const char* canon_PIXMA_iP7200_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP7200_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP7200_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_iP7200_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP7200_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP7200_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_iP7200_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP7200_modeuses[] = {
  { "Plain",            canon_PIXMA_iP7200_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_iP7200_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_iP7200_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_iP7200_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP7200_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP7200_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_iP7200_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_iP7200_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_iP7200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_iP7200_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_iP7200_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_iP7200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_iP7200_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP7200_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP7200_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP7200_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_iP7200_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP7200);

/* ----------------------------------- Canon iP7500  ----------------------------------- */
static const char* canon_PIXMA_iP7500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest modes not yet supported */
static const char* canon_PIXMA_iP7500_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};


static const char* canon_PIXMA_iP7500_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP7500_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP7500_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_PIXMA_iP7500_modeuses_FA[] = {
  "600x600dpi_photohigh3",
  NULL
};

static const char* canon_PIXMA_iP7500_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_PIXMA_iP7500_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iP7500_modeuses_Transparency[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP7500_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP7500_modeuses[] = {
 { "Plain",             canon_PIXMA_iP7500_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_PIXMA_iP7500_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP7500_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP7500_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "Hagaki", 	        canon_PIXMA_iP7500_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "FineArtPhotoRag", 	canon_PIXMA_iP7500_modeuses_FA, 0 },
 { "FineArtOther", 	canon_PIXMA_iP7500_modeuses_FA, 0 },
 { "DiscCompat",	canon_PIXMA_iP7500_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP7500_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP7500_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP7500_modeuses_Hagaki, 0 },
 { "Transparency",	canon_PIXMA_iP7500_modeuses_Transparency, 0 },/*untested*/
 { "PhotopaperOther",	canon_PIXMA_iP7500_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP7500);

/* ----------------------------------- Canon iP8100  ----------------------------------- */
static const char* canon_PIXMA_iP8100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2", /* duplex */
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  /* Mono */
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  NULL
  };

/* no support for modes using R yet */
static const char* canon_PIXMA_iP8100_modeuses_PPpro[] = {
  "600x600dpi_photohigh", /*untested*/
  "600x600dpi_photo",
  NULL
};


static const char* canon_PIXMA_iP8100_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  /* Mono */
  "600x600dpi_mono",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_FA[] = {
  "600x600dpi_photohigh3",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_disc[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_TShirt[] = {
  "600x600dpi",
  NULL
};

static const char* canon_PIXMA_iP8100_modeuses_Transparency[] = {
  "600x600dpi_std4",
  "600x600dpi_draft4",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iP8100_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft2",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iP8100_modeuses[] = {
 { "Plain",             canon_PIXMA_iP8100_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP8100_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP8100_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP8100_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP8100_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP8100_modeuses_PPgloss, 0 },
 { "Coated",		canon_PIXMA_iP8100_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP8100_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iP8100_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "FineArtPhotoRag", 	canon_PIXMA_iP8100_modeuses_FA, 0 },
 { "FineArtOther", 	canon_PIXMA_iP8100_modeuses_FA, 0 },
 { "DiscCompat",	canon_PIXMA_iP8100_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP8100_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP8100_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP8100_modeuses_Hagaki, 0 },
 { "Transparency",	canon_PIXMA_iP8100_modeuses_Transparency, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP8100_modeuses_PPother, 0 }, /*untested*/
 };

DECLARE_MODEUSES(canon_PIXMA_iP8100);

/* ----------------------------------- Canon iP8500  ----------------------------------- */
static const char* canon_PIXMA_iP8500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  NULL
  };

/* modes not yet supported */
static const char* canon_PIXMA_iP8500_modeuses_PPpro[] = {
  "600x600dpi_photodraft", /*temporary stand-in: untested*/
  NULL
};

/* most modes unsupported */
static const char* canon_PIXMA_iP8500_modeuses_PPplus[] = {
  "600x600dpi_photodraft",
  NULL
};

/* most modes unsupported */
static const char* canon_PIXMA_iP8500_modeuses_PPplusDS[] = {
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP8500_modeuses_PPhires[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* US driver does not support this media: untested */
static const char* canon_PIXMA_iP8500_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* US driver does not support this media */
static const char* canon_PIXMA_iP8500_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  NULL
};

static const char* canon_PIXMA_iP8500_modeuses_disc[] = {
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_PIXMA_iP8500_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_iP8500_modeuses_Transparency[] = {
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_PIXMA_iP8500_modeuses_PPother[] = {
  "600x600dpi_photo3",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP8500_modeuses[] = {
  { "Plain",            canon_PIXMA_iP8500_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "GlossyPro",	canon_PIXMA_iP8500_modeuses_PPpro, 0 }, /*unsupported*/
  { "PhotopaperPlus",	canon_PIXMA_iP8500_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP8500_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP8500_modeuses_PPpro, 0 }, /*unsupported*/
  { "GlossyPaper",	canon_PIXMA_iP8500_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP8500_modeuses_PPhires, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP8500_modeuses_inkjetHagaki, 0 }, /*untested*/
  { "Hagaki", 	        canon_PIXMA_iP8500_modeuses_Hagaki, DUPLEX_SUPPORT },/*untested*/
  { "DiscCompat",	canon_PIXMA_iP8500_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP8500_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP8500_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP8500_modeuses_Hagaki, 0 },
  { "Transparency",	canon_PIXMA_iP8500_modeuses_Transparency, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP8500_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP8500);

/* ----------------------------------- Canon iP8600  ----------------------------------- */
static const char* canon_PIXMA_iP8600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2", /* duplex */
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  /* Mono */
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  NULL
  };

/* modes using R not yet supported */
static const char* canon_PIXMA_iP8600_modeuses_PPpro[] = {
  "600x600dpi_photohigh2", /*untested*/
  "600x600dpi_photo",
  NULL
};

/* highest mode using R not supported yet */
static const char* canon_PIXMA_iP8600_modeuses_PPplus[] = {
  "600x600dpi_photohigh2", /*untested*/
  "600x600dpi_photo",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  /* Mono */
  "600x600dpi_mono",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_FA[] = {
  "600x600dpi_photohigh4",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_disc[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_TShirt[] = {
  "600x600dpi",
  NULL
};

static const char* canon_PIXMA_iP8600_modeuses_Transparency[] = {
  "600x600dpi_std4",
  "600x600dpi_draft4",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iP8600_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP8600_modeuses[] = {
 { "Plain",             canon_PIXMA_iP8600_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP8600_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP8600_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP8600_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP8600_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP8600_modeuses_PPplusDS, 0 },
 { "Coated",		canon_PIXMA_iP8600_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP8600_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iP8600_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "FineArtPhotoRag", 	canon_PIXMA_iP8600_modeuses_FA, 0 },
 { "FineArtOther", 	canon_PIXMA_iP8600_modeuses_FA, 0 },
 { "DiscCompat",	canon_PIXMA_iP8600_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP8600_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP8600_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP8600_modeuses_Hagaki, 0 },
 { "Transparency",	canon_PIXMA_iP8600_modeuses_Transparency, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP8600_modeuses_PPother, 0 }, /*untested*/
 };

DECLARE_MODEUSES(canon_PIXMA_iP8600);

/* ----------------------------------- Canon iP8700  ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_iP8700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  NULL
  };

/* unsupported*/
static const char* canon_PIXMA_iP8700_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_iP8700_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_iP8700_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iP8700_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iP8700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_iP8700_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP8700_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_iP8700_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_iP8700_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP8700_modeuses[] = {
  { "Plain",            canon_PIXMA_iP8700_modeuses_plain, 0},
  { "PhotoPlusGloss2",  canon_PIXMA_iP8700_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_iP8700_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_iP8700_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP8700_modeuses_PPplusG2, 0 },
  { "GlossyPaperStandard",canon_PIXMA_iP8700_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP8700_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_iP8700_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_iP8700_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_iP8700_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP8700_modeuses_inkjetHagaki, 0 },
  { "InkjetPhotoHagakiK",canon_PIXMA_iP8700_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_iP8700_modeuses_Hagaki, 0 },
  { "DiscCompat",	canon_PIXMA_iP8700_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP8700_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP8700_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP8700_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_iP8700_modeuses_photorag, 0 },/*untested*/
  { "FineArtOther",     canon_PIXMA_iP8700_modeuses_photorag, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP8700_modeuses_PPother, 0 },
};
    
DECLARE_MODEUSES(canon_PIXMA_iP8700);

/* ----------------------------------- Canon iP9910  ----------------------------------- */
static const char* canon_PIXMA_iP9910_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2", /*duplex*/
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  /* Mono */
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  NULL
  };

/* highest mode using R not yet supported */
static const char* canon_PIXMA_iP9910_modeuses_PPpro[] = {
  "600x600dpi_photo",
  "600x600dpi_photo2",
  NULL
};

/* highest mode using R not supported yet */
static const char* canon_PIXMA_iP9910_modeuses_PPplus[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  /* Mono */
  "600x600dpi_mono",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_FA[] = {
  "600x600dpi_photohigh4",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_disc[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_TShirt[] = {
  "600x600dpi",
  NULL
};

static const char* canon_PIXMA_iP9910_modeuses_Transparency[] = {
  "600x600dpi_std4",
  "600x600dpi_draft4",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iP9910_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iP9910_modeuses[] = {
 { "Plain",             canon_PIXMA_iP9910_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP9910_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP9910_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP9910_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP9910_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP9910_modeuses_PPplusDS, 0 },
 { "Coated",		canon_PIXMA_iP9910_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP9910_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iP9910_modeuses_Hagaki, 0 },
 { "FineArtPhotoRag", 	canon_PIXMA_iP9910_modeuses_FA, 0 },
 { "FineArtOther", 	canon_PIXMA_iP9910_modeuses_FA, 0 },
 { "DiscCompat",	canon_PIXMA_iP9910_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP9910_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP9910_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP9910_modeuses_Hagaki, 0 },
 { "Transparency",	canon_PIXMA_iP9910_modeuses_Transparency, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP9910_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP9910);

/* ----------------------------------- Canon iX4000  ----------------------------------- */
static const char* canon_PIXMA_iX4000_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not supported yet */
static const char* canon_PIXMA_iX4000_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iX4000_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iX4000_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iX4000_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iX4000_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iX4000_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* US driver does not have this media */
static const char* canon_PIXMA_iX4000_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

/* US driver does not have this media */
static const char* canon_PIXMA_iX4000_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iX4000_modeuses[] = {
 { "Plain",             canon_PIXMA_iX4000_modeuses_plain, 0 },
 { "GlossyPro",	        canon_PIXMA_iX4000_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iX4000_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iX4000_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iX4000_modeuses_PPplusDS, 0 },
 { "GlossyPaper",	canon_PIXMA_iX4000_modeuses_PPplusDS, 0 },
 { "Coated",		canon_PIXMA_iX4000_modeuses_PPplusDS, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iX4000_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iX4000_modeuses_Hagaki, 0 },
 { "TShirt",		canon_PIXMA_iX4000_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iX4000_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iX4000_modeuses_PPother, 0 },/*Note: US driver does not have this media*/
 { "Transparency",	canon_PIXMA_iX4000_modeuses_Transparency, 0 },/*Note: US driver does not have this media */
 };

DECLARE_MODEUSES(canon_PIXMA_iX4000);

/* ----------------------------------- Canon iX5000  ----------------------------------- */
static const char* canon_PIXMA_iX5000_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not supported yet */
static const char* canon_PIXMA_iX5000_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iX5000_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iX5000_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iX5000_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iX5000_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iX5000_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* US driver does not have this media */
static const char* canon_PIXMA_iX5000_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_iX5000_modeuses[] = {
 { "Plain",             canon_PIXMA_iX5000_modeuses_plain, 0 },
 { "GlossyPro",	        canon_PIXMA_iX5000_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iX5000_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iX5000_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iX5000_modeuses_PPplusDS, 0 },
 { "GlossyPaper",	canon_PIXMA_iX5000_modeuses_PPplusDS, 0 },
 { "Coated",		canon_PIXMA_iX5000_modeuses_PPplusDS, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iX5000_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iX5000_modeuses_Hagaki, 0 },
 { "TShirt",		canon_PIXMA_iX5000_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iX5000_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iX5000_modeuses_PPother, 0 },/*Note: US driver does not have this media*/
 { "Transparency",	canon_PIXMA_iX5000_modeuses_inkjetHagaki, 0 },/*Note: US driver does not have this media */
 };

DECLARE_MODEUSES(canon_PIXMA_iX5000);

/* ----------------------------------- Canon iX6500  ----------------------------------- */
static const char* canon_PIXMA_iX6500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };


static const char* canon_PIXMA_iX6500_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not supported yet */
static const char* canon_PIXMA_iX6500_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iX6500_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iX6500_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iX6500_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iX6500_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/*untested*/
static const char* canon_PIXMA_iX6500_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iX6500_modeuses[] = {
  { "Plain",            canon_PIXMA_iX6500_modeuses_plain, 0 },
  { "PhotoPlusGloss2",	canon_PIXMA_iX6500_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",	canon_PIXMA_iX6500_modeuses_PPproPlat, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iX6500_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iX6500_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iX6500_modeuses_PPmatte, 0 },
  { "Coated",		canon_PIXMA_iX6500_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iX6500_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_iX6500_modeuses_PPplusG2, 0 },
  { "HagakiA", 	        canon_PIXMA_iX6500_modeuses_Hagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iX6500_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_iX6500_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iX6500_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iX6500_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_PIXMA_iX6500);

/* ----------------------------------- Canon iX6800  ----------------------------------- */

static const char* canon_PIXMA_iX6800_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_iX6800_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iX6800_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_iX6800_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_iX6800_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_iX6800_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iX6800_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_iX6800_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iX6800_modeuses[] = {
  { "Plain",            canon_PIXMA_iX6800_modeuses_plain, 0},
  { "PhotoPlusGloss2",  canon_PIXMA_iX6800_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_iX6800_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_iX6800_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iX6800_modeuses_PPplusG2, 0 },
  { "GlossyPaperStandard",canon_PIXMA_iX6800_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iX6800_modeuses_PPmatte, 0 },
  { "Coated",		canon_PIXMA_iX6800_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_iX6800_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iX6800_modeuses_inkjetHagaki, 0 },
  { "InkjetPhotoHagakiK",canon_PIXMA_iX6800_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_iX6800_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_iX6800_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iX6800_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_iX6800_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iX6800);

/* ----------------------------------- Canon iX7000  ----------------------------------- */
static const char* canon_PIXMA_iX7000_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };


static const char* canon_PIXMA_iX7000_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iX7000_modeuses_PPproPlat[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

/* highest mode not supported yet */
static const char* canon_PIXMA_iX7000_modeuses_PPsemigloss[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_iX7000_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iX7000_modeuses_Hagaki[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_PIXMA_iX7000_modeuses_FA[] = {
  "600x600dpi_photo3",
  NULL
};

static const canon_modeuse_t canon_PIXMA_iX7000_modeuses[] = {
  { "Plain",            canon_PIXMA_iX7000_modeuses_plain, DUPLEX_SUPPORT },
  { "PhotoPlusGloss2",	canon_PIXMA_iX7000_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",	canon_PIXMA_iX7000_modeuses_PPproPlat, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iX7000_modeuses_PPsemigloss, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iX7000_modeuses_PPproPlat, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iX7000_modeuses_inkjetHagaki, 0 },
  { "HagakiA", 	        canon_PIXMA_iX7000_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_iX7000_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "FineArtPhotoRag", 	canon_PIXMA_iX7000_modeuses_FA, 0 },
  { "FineArtOther", 	canon_PIXMA_iX7000_modeuses_FA, 0 },
  { "Envelope",		canon_PIXMA_iX7000_modeuses_Hagaki, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iX7000);

/* ----------------------------------- Canon MP5  ----------------------------------- */
static const char* canon_PIXMA_MP5_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",/*untested*/
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_MP5_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MP5_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MP5_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MP5_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_PIXMA_MP5_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MP5_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_stdmono2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_PIXMA_MP5_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MP5_modeuses_PPother[] = {
  "600x600dpi_photohigh2",/*untested*/
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_PIXMA_MP5_modeuses[] = {
  { "Plain",            canon_PIXMA_MP5_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_MP5_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_MP5_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MP5_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MP5_modeuses_PPgloss, 0 },
  { "Coated",		canon_PIXMA_MP5_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_MP5_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_MP5_modeuses_Hagaki, 0 },
  { "GlossyFilm", 	canon_PIXMA_MP5_modeuses_PPmatte, 0 },
  { "TShirt",		canon_PIXMA_MP5_modeuses_TShirt, 0 },
  { "Transparency",	canon_PIXMA_MP5_modeuses_Transparency, 0 },
  { "Envelope",		canon_PIXMA_MP5_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_MP5_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MP5);

/* ----------------------------------- Canon MP55  ----------------------------------- */
static const char* canon_PIXMA_MP55_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_medium",
  "600x600dpi",
  "300x300dpi",/*untested*/
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_MP55_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_HiRes[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MP55_modeuses_PPother[] = {
  "600x600dpi_photohigh",/*untested*/
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_PIXMA_MP55_modeuses[] = {
  { "Plain",            canon_PIXMA_MP55_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_MP55_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_MP55_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MP55_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MP55_modeuses_PPgloss, 0 },
  { "Coated",		canon_PIXMA_MP55_modeuses_HiRes, 0 },
  { "InkJetHagaki", 	canon_PIXMA_MP55_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_MP55_modeuses_Hagaki, 0 },
  { "GlossyFilm", 	canon_PIXMA_MP55_modeuses_PPpro, 0 },
  { "TShirt",		canon_PIXMA_MP55_modeuses_TShirt, 0 },
  { "Transparency",	canon_PIXMA_MP55_modeuses_Transparency, 0 },
  { "Envelope",		canon_PIXMA_MP55_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_MP55_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MP55);

/* ----------------------------------- Canon MPC400  ----------------------------------- */
static const char* canon_PIXMA_MPC400_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",/*untested*/
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_MPC400_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

/* experimental */
static const char* canon_PIXMA_MPC400_modeuses_PPmatte[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_PIXMA_MPC400_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MPC400_modeuses_HiRes[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

/* experimental */
static const char* canon_PIXMA_MPC400_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MPC400_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_PIXMA_MPC400_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MPC400_modeuses_Env[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

/* experimental */
static const char* canon_PIXMA_MPC400_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MPC400_modeuses[] = {
  { "Plain",            canon_PIXMA_MPC400_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_MPC400_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MPC400_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MPC400_modeuses_PPgloss, 0 },
  { "Coated",		canon_PIXMA_MPC400_modeuses_HiRes, 0 },
  { "InkJetHagaki", 	canon_PIXMA_MPC400_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_MPC400_modeuses_Env, 0 },
  { "GlossyFilm", 	canon_PIXMA_MPC400_modeuses_PPpro, 0 },
  { "GlossyCard", 	canon_PIXMA_MPC400_modeuses_PPgloss, 0 },
  { "TShirt",		canon_PIXMA_MPC400_modeuses_TShirt, 0 },
  { "Transparency",	canon_PIXMA_MPC400_modeuses_Transparency, 0 },
  { "Envelope",		canon_PIXMA_MPC400_modeuses_Env, 0 },
  { "PhotopaperOther",	canon_PIXMA_MPC400_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MPC400);

/* ----------------------------------- Canon MP150  ----------------------------------- */
static const char* canon_MULTIPASS_MP150_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* color-only */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",
  NULL
  };

static const char* canon_MULTIPASS_MP150_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh3",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",
  /* mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  "600x600dpi_draftmono2",
  /* color-only */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP150_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP150_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "GlossyPro",	canon_MULTIPASS_MP150_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlus",	canon_MULTIPASS_MP150_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP150_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MP150_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MP150_modeuses_PPgloss, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MP150_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MP150_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MP150_modeuses_Hagaki, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MP150_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MP150_modeuses_Hagaki, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MP150_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP150);

/* ----------------------------------- Canon MP190  ----------------------------------- */
static const char* canon_MULTIPASS_MP190_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",
  NULL
  };

static const char* canon_MULTIPASS_MP190_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP190_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP190_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP190_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  "600x600dpi_draftmono2",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP190_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP190_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP190_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP190_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP190_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoPro2",	canon_MULTIPASS_MP190_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "GlossyPro",	canon_MULTIPASS_MP190_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlus",	canon_MULTIPASS_MP190_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MP190_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MP190_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MP190_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MP190_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MP190_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MP190_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MP190_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MP190_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MP190_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP190);

/* ----------------------------------- Canon MP210  ----------------------------------- */
static const char* canon_MULTIPASS_MP210_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",
  NULL
  };

static const char* canon_MULTIPASS_MP210_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested here*/
  NULL
};

static const char* canon_MULTIPASS_MP210_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP210_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested here*/
  NULL
};

static const char* canon_MULTIPASS_MP210_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested here*/
  NULL
};

static const char* canon_MULTIPASS_MP210_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  "600x600dpi_draftmono2",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP210_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP210_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP210_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP210_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "GlossyPro",	canon_MULTIPASS_MP210_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlus",	canon_MULTIPASS_MP210_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP210_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MP210_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MP210_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MP210_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MP210_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MP210_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MP210_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MP210_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MP210_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP210);

/* ----------------------------------- Canon MP230  ----------------------------------- */
static const char* canon_MULTIPASS_MP230_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",/*untested*/
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",/*untested*/
  NULL
  };

static const char* canon_MULTIPASS_MP230_modeuses_PPproPlat[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP230_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP230_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  "600x600dpi_draftmono2",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP230_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP230_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP230_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP230_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
 { "PhotoPlusGloss2",	canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MP230_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
 { "PhotoProLuster",    canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "HagakiA", 	        canon_MULTIPASS_MP230_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "InkJetHagaki", 	canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "CanonPhotoHagakiK", canon_MULTIPASS_MP230_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MP230_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP230_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP230_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP230_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP230);

/* ----------------------------------- Canon MP240  ----------------------------------- */
static const char* canon_MULTIPASS_MP240_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",
  NULL
  };

static const char* canon_MULTIPASS_MP240_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP240_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP240_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  "600x600dpi_draftmono2",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP240_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP240_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP240_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP240_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
 { "PhotoPlusGloss2",	canon_MULTIPASS_MP240_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoPro2",	        canon_MULTIPASS_MP240_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MP240_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MP240_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP240_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP240_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP240_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "InkJetHagaki", 	canon_MULTIPASS_MP240_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MP240_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP240_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP240_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP240_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP240);

/* ----------------------------------- Canon MP250  ----------------------------------- */
static const char* canon_MULTIPASS_MP250_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  /* mono */
  "600x600dpi_highmono",
  "600x600dpi_highmono2",/* duplex */
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",/*untested*/
  /* color-only --- no special duplex */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",/*untested*/
  NULL
  };

static const char* canon_MULTIPASS_MP250_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP250_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP250_modeuses_Hagaki[] = {
  "600x600dpi_high5",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  /* mono */
  "600x600dpi_highmono5",
  "600x600dpi_mono2",
  "600x600dpi_draftmono2",/*untested*/
  /* color-only */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP250_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP250_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP250_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP250_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotoPro2",	canon_MULTIPASS_MP250_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MP250_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "ProPhotoHagakiP",  canon_MULTIPASS_MP250_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MP250_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT},
  { "TShirt",		canon_MULTIPASS_MP250_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MP250_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MP250_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP250);

/* ----------------------------------- Canon MP280  ----------------------------------- */
static const char* canon_MULTIPASS_MP280_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",/*untested*/
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",/*untested*/
  NULL
  };

static const char* canon_MULTIPASS_MP280_modeuses_PPproPlat[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP280_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP280_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  "600x600dpi_draftmono2",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP280_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP280_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP280_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP280_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
 { "PhotoPlusGloss2",	canon_MULTIPASS_MP280_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MP280_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MP280_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP280_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP280_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP280_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "HagakiA", 	        canon_MULTIPASS_MP280_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "InkJetHagaki", 	canon_MULTIPASS_MP280_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "CanonPhotoHagakiK", canon_MULTIPASS_MP280_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MP280_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP280_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP280_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP280_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP280);

/* ----------------------------------- Canon MP360  ----------------------------------- */
static const char* canon_MULTIPASS_MP360_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_highmono",/* mono */
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_MULTIPASS_MP360_modeuses_PPpro[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP360_modeuses_PPplus[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP360_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP360_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP360_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_highmono2",/* mono */
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_MULTIPASS_MP360_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP360_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP360_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP360_modeuses_plain, 0 },
  { "GlossyPro",	canon_MULTIPASS_MP360_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP360_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP360_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP360_modeuses_PPgloss, 0 },
  { "Coated",		canon_MULTIPASS_MP360_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP360_modeuses_PPmatte, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP360_modeuses_Hagaki, 0 },
  { "TShirt",		canon_MULTIPASS_MP360_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP360_modeuses_Hagaki, 0 },
  { "Transparency",	canon_MULTIPASS_MP360_modeuses_Transparency, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP360);

/* ----------------------------------- Canon MP470  ----------------------------------- */
static const char* canon_MULTIPASS_MP470_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  "300x300dpi_std2",
  "300x300dpi_draft2",
  NULL
  };

/* high mode not supported yet */
static const char* canon_MULTIPASS_MP470_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP470_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP470_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP470_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP470_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  /* Mono */
  "600x600dpi_highmono3",
  "600x600dpi_mono3",
  "600x600dpi_draftmono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",
  NULL
};

static const char* canon_MULTIPASS_MP470_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP470_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP470_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP470_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
  { "GlossyPro",	canon_MULTIPASS_MP470_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlus",	canon_MULTIPASS_MP470_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP470_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP470_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MP470_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MP470_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MP470_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MP470_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MP470_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MP470_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MP470_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MP470_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP470);

/* ----------------------------------- Canon MP480  ----------------------------------- */
static const char* canon_MULTIPASS_MP480_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  "300x300dpi_std2",
  "300x300dpi_draft2",
  NULL
  };

static const char* canon_MULTIPASS_MP480_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP480_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP480_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* Mono */
  "600x600dpi_highmono3",
  "600x600dpi_mono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_MULTIPASS_MP480_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP480_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP480_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP480_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
 { "PhotoPlusGloss2",	canon_MULTIPASS_MP480_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoPro2",	        canon_MULTIPASS_MP480_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MP480_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP480_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP480_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP480_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "InkJetHagaki", 	canon_MULTIPASS_MP480_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MP480_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP480_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP480_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP480_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP480);

/* ----------------------------------- Canon MP490  ----------------------------------- */
static const char* canon_MULTIPASS_MP490_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  "300x300dpi_mono",
  "300x300dpi_draftmono",/*untested*/
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  "300x300dpi_std3",
  "300x300dpi_draft3",/*untested*/
  NULL
  };

static const char* canon_MULTIPASS_MP490_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP490_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP490_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_mono2",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_MULTIPASS_MP490_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP490_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  "600x600dpi_photodraft",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP490_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP490_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
 { "PhotoPro2",	        canon_MULTIPASS_MP490_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoPlusGloss2",	canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "InkJetHagaki", 	canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "ProPhotoHagakiP", 	canon_MULTIPASS_MP490_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "CanonPhotoHagakiK", canon_MULTIPASS_MP490_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "HagakiA", 	        canon_MULTIPASS_MP490_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "Hagaki", 	        canon_MULTIPASS_MP490_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP490_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP490_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP490_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP490);

/* ----------------------------------- Canon MP493  ----------------------------------- */
static const char* canon_MULTIPASS_MP493_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  "300x300dpi_std2",
  "300x300dpi_draft2",
  NULL
  };

static const char* canon_MULTIPASS_MP493_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP493_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP493_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* Mono */
  "600x600dpi_highmono3",
  "600x600dpi_mono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_MULTIPASS_MP493_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP493_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP493_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP493_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
 { "PhotoPlusGloss2",	canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoPro2",	        canon_MULTIPASS_MP493_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "HagakiA", 	        canon_MULTIPASS_MP493_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "InkJetHagaki", 	canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "CanonPhotoHagakiK", canon_MULTIPASS_MP493_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "ProPhotoHagakiP",   canon_MULTIPASS_MP493_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MP493_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP493_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP493_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP493_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP493);

/* ----------------------------------- Canon MP520  ----------------------------------- */
static const char* canon_MULTIPASS_MP520_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP520_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP520_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP520_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP520_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_MULTIPASS_MP520_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MP520_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP520_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP520_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP520_modeuses_plain, 0 },
  { "GlossyPro",	canon_MULTIPASS_MP520_modeuses_PPpro, 0 },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP520_modeuses_PPplus, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP520_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP520_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP520_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP520_modeuses_PPplusDS, 0 },
  { "Coated",		canon_MULTIPASS_MP520_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP520_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP520_modeuses_Hagaki, 0 },
  { "TShirt",		canon_MULTIPASS_MP520_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP520_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP520_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP520);

/* ----------------------------------- Canon MP530  ----------------------------------- */
static const char* canon_MULTIPASS_MP530_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* most photo modes not yet supported */
static const char* canon_MULTIPASS_MP530_modeuses_PPpro[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

/* only draft available */
static const char* canon_MULTIPASS_MP530_modeuses_PPplus[] = {
  "600x600dpi_photodraft",
  NULL
};

/*most photo modes not yet supported */
static const char* canon_MULTIPASS_MP530_modeuses_PPplusDS[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP530_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP530_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP530_modeuses_disc[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP530_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP530_modeuses_PPother[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP530_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP530_modeuses_plain, 0 },
  { "GlossyPro",	canon_MULTIPASS_MP530_modeuses_PPpro, 0 },/*not supported yet*/
  { "PhotopaperPlus",	canon_MULTIPASS_MP530_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP530_modeuses_PPplusDS, 0 },/*not supported yet*/
  { "GlossyPaper",	canon_MULTIPASS_MP530_modeuses_PPplusDS, 0 },/*not supported yet*/
  { "PhotopaperMatte",	canon_MULTIPASS_MP530_modeuses_PPplusDS, 0 },/*not supported yet*/
  { "Coated",		canon_MULTIPASS_MP530_modeuses_PPplusDS, 0 },/*not supported yet*/
  { "InkJetHagaki", 	canon_MULTIPASS_MP530_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP530_modeuses_Hagaki, 0 },
  { "CD",   	        canon_MULTIPASS_MP530_modeuses_plain, 0 },/*NOTE:temporary replacement*/
  { "DiscCompat",	canon_MULTIPASS_MP530_modeuses_disc, 0 },/*not supported yet*/
  { "DiscOthers",	canon_MULTIPASS_MP530_modeuses_disc, 0 },/*not supported yet*/
  { "TShirt",		canon_MULTIPASS_MP530_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP530_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP530_modeuses_PPother, 0 },/*not supported yet*/
};

DECLARE_MODEUSES(canon_MULTIPASS_MP530);

/* ----------------------------------- Canon MP540  ----------------------------------- */
static const char* canon_MULTIPASS_MP540_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP540_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP540_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP540_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP540_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP540_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP540_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP540_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP540_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP540_modeuses_plain, 0 },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP540_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP540_modeuses_PPpro, 0 },
  { "PhotoProPlat",     canon_MULTIPASS_MP540_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP540_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP540_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss", canon_MULTIPASS_MP540_modeuses_PPplusG2, 0 },
  { "Coated",		canon_MULTIPASS_MP540_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP540_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP540_modeuses_Hagaki, 0 },
  { "TShirt",		canon_MULTIPASS_MP540_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP540_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP540_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP540);

/* ----------------------------------- Canon MP550  ----------------------------------- */
static const char* canon_MULTIPASS_MP550_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  NULL
  };

/*highest mode not supported yet*/
static const char* canon_MULTIPASS_MP550_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP550_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP550_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP550_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP550_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP550_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP550_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP550_modeuses[] = {
  { "Plain",		canon_MULTIPASS_MP550_modeuses_plain, 0 },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP550_modeuses_PPpro, 0 },
  { "PhotoProPlat",	canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP550_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP550_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_MULTIPASS_MP550_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP550_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
  { "ProPhotoHagakiP",  canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP550_modeuses_Hagaki, 0 },
  { "TShirt",		canon_MULTIPASS_MP550_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP550_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP550_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP550);

/* ----------------------------------- Canon MP560  ----------------------------------- */
static const char* canon_MULTIPASS_MP560_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/* duplex */
  "600x600dpi",
  "300x300dpi",
  NULL
  };

/*highest mode not supported yet*/
static const char* canon_MULTIPASS_MP560_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP560_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP560_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP560_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MP560_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP560_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP560_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP560_modeuses[] = {
  { "Plain",		canon_MULTIPASS_MP560_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MP560_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP560_modeuses_PPpro, 0 },
  { "PhotoProPlat",	canon_MULTIPASS_MP560_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MP560_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP560_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP560_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP560_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_MULTIPASS_MP560_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MP560_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MP560_modeuses_PPplusG2, 0 },
  { "ProPhotoHagakiP",  canon_MULTIPASS_MP560_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP560_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "TShirt",		canon_MULTIPASS_MP560_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP560_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP560_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP560);

/* ----------------------------------- Canon MP600  ----------------------------------- */
static const char* canon_MULTIPASS_MP600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/*unsupported*/
static const char* canon_MULTIPASS_MP600_modeuses_PPpro[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
};

/*most modes unsupported*/
static const char* canon_MULTIPASS_MP600_modeuses_PPplus[] = {
  "600x600dpi_photodraft",
  NULL
};

/*unsupported*/
static const char* canon_MULTIPASS_MP600_modeuses_PPplusDS[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
};

/*unsupported*/
static const char* canon_MULTIPASS_MP600_modeuses_PPmatte[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
};

/*highest mode unsupported*/
static const char* canon_MULTIPASS_MP600_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP600_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

/*unsupported*/
static const char* canon_MULTIPASS_MP600_modeuses_disc[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP600_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/*unsupported*/
static const char* canon_MULTIPASS_MP600_modeuses_PPother[] = {
  "600x600dpi_photodraft",/*stand-in*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP600_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP600_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_MULTIPASS_MP600_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_MULTIPASS_MP600_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP600_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_MULTIPASS_MP600_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_MULTIPASS_MP600_modeuses_PPplusDS, 0 },
 { "Coated",		canon_MULTIPASS_MP600_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_MULTIPASS_MP600_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP600_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "CD",	        canon_MULTIPASS_MP600_modeuses_plain, 0 },/*temporary for plain modes*/
 { "DiscCompat",	canon_MULTIPASS_MP600_modeuses_disc, 0 },
 { "DiscOthers",	canon_MULTIPASS_MP600_modeuses_disc, 0 },
 { "TShirt",		canon_MULTIPASS_MP600_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP600_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP600_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP600);

/* ----------------------------------- Canon MP610  ----------------------------------- */
static const char* canon_MULTIPASS_MP610_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* missing highest resolution mode as not yet implemented */
static const char* canon_MULTIPASS_MP610_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP610_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP610_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP610_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_MULTIPASS_MP610_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_MULTIPASS_MP610_modeuses_PPplus, 0 },
 { "PhotoPlusGloss2",   canon_MULTIPASS_MP610_modeuses_PPplusG2, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP610_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_MULTIPASS_MP610_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_MULTIPASS_MP610_modeuses_PPplusDS, 0 },
 { "Coated",		canon_MULTIPASS_MP610_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_MULTIPASS_MP610_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP610_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "DiscCompat",	canon_MULTIPASS_MP610_modeuses_disc, 0 },
 { "DiscOthers",	canon_MULTIPASS_MP610_modeuses_disc, 0 },
 { "TShirt",		canon_MULTIPASS_MP610_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP610_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP610_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP610);

/* ----------------------------------- Canon MP620  ----------------------------------- */
static const char* canon_MULTIPASS_MP620_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP620_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP620_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP620_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP620_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP620_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP620_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP620_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP620_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP620_modeuses_plain, 0 },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP620_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP620_modeuses_PPpro, 0 },
  { "PhotoProPlat",     canon_MULTIPASS_MP620_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP620_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP620_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss", canon_MULTIPASS_MP620_modeuses_PPplusG2, 0 },
  { "Coated",		canon_MULTIPASS_MP620_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP620_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP620_modeuses_Hagaki, 0 },
  { "TShirt",		canon_MULTIPASS_MP620_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP620_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP620_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP620);

/* ----------------------------------- Canon MP630  ----------------------------------- */
static const char* canon_MULTIPASS_MP630_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP630_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP630_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP630_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP630_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP630_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP630_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MP630_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP630_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP630_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP630_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP630_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP630_modeuses_PPpro, 0 },
  { "PhotoProPlat",     canon_MULTIPASS_MP630_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss", canon_MULTIPASS_MP630_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP630_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP630_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP630_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP630_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP630_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat", 	canon_MULTIPASS_MP630_modeuses_disc, 0 },
  { "DiscOthers", 	canon_MULTIPASS_MP630_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP630_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP630_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP630_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP630);

/* ----------------------------------- Canon MP640  ----------------------------------- */
static const char* canon_MULTIPASS_MP640_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP640_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP640_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP640_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP640_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP640_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP640_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MP640_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP640_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP640_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP640_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP640_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP640_modeuses_PPpro, 0 },
  { "PhotoProPlat",     canon_MULTIPASS_MP640_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss", canon_MULTIPASS_MP640_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP640_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP640_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP640_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_MULTIPASS_MP640_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MP640_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MP640_modeuses_PPplusG2, 0 },
  { "ProPhotoHagakiP",  canon_MULTIPASS_MP640_modeuses_PPpro, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP640_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat", 	canon_MULTIPASS_MP640_modeuses_disc, 0 },
  { "DiscOthers", 	canon_MULTIPASS_MP640_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP640_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP640_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP640_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP640);

/* ----------------------------------- Canon MP700  ----------------------------------- */
static const char* canon_MULTIPASS_MP700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",/*untested*/
  "600x600dpi_draft",
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP700_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP700_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP700_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",/*untested*/
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP700_modeuses_Hagaki[] = {
  "600x600dpi_high3",/*untested*/
  "600x600dpi_high2",
  "600x600dpi_std2",/*untested*/
  "600x600dpi_draft2",
  NULL
};

static const char* canon_MULTIPASS_MP700_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP700_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP700_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

/*untested*/
static const char* canon_MULTIPASS_MP700_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP700_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP700_modeuses_plain, 0 },
  { "GlossyPro",	canon_MULTIPASS_MP700_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP700_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP700_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP700_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP700_modeuses_PPplus, 0 },
  { "Coated",		canon_MULTIPASS_MP700_modeuses_coated, 0 },
  { "GlossyFilm",	canon_MULTIPASS_MP700_modeuses_PPplus, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP700_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP700_modeuses_Hagaki, 0 },
  { "DiscCompat",	canon_MULTIPASS_MP700_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP700_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP700_modeuses_TShirt, 0 },
  { "Transparency",	canon_MULTIPASS_MP700_modeuses_Transparency, 0 },
  { "Envelope",		canon_MULTIPASS_MP700_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP700_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_MULTIPASS_MP700);

/* ----------------------------------- Canon MP710  ----------------------------------- */
static const char* canon_MULTIPASS_MP710_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",/*untested*/
  "600x600dpi_draft",
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP710_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP710_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP710_modeuses_coated[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP710_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",/*untested*/
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP710_modeuses_Hagaki[] = {
  "600x600dpi_high3",/*untested*/
  "600x600dpi_high2",
  "600x600dpi_std2",/*untested*/
  "600x600dpi_draft2",
  NULL
};

static const char* canon_MULTIPASS_MP710_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP710_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP710_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

/*untested*/
static const char* canon_MULTIPASS_MP710_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP710_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP710_modeuses_plain, 0 },
  { "GlossyPro",	canon_MULTIPASS_MP710_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP710_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP710_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP710_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP710_modeuses_PPplus, 0 },
  { "Coated",		canon_MULTIPASS_MP710_modeuses_coated, 0 },
  { "GlossyFilm",	canon_MULTIPASS_MP710_modeuses_PPplus, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP710_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP710_modeuses_Hagaki, 0 },
  { "DiscCompat",	canon_MULTIPASS_MP710_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP710_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP710_modeuses_TShirt, 0 },
  { "Transparency",	canon_MULTIPASS_MP710_modeuses_Transparency, 0 },
  { "Envelope",		canon_MULTIPASS_MP710_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP710_modeuses_PPother, 0 },/*untested*/
};

DECLARE_MODEUSES(canon_MULTIPASS_MP710);

/* ----------------------------------- Canon MP750  ----------------------------------- */
static const char* canon_MULTIPASS_MP750_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP750_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP750_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP750_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP750_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MP750_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP750_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP750_modeuses_Transparency[] = {
  "600x600dpi_ohphigh",
  "600x600dpi_ohp",
  NULL
};

static const char* canon_MULTIPASS_MP750_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP750_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP750_modeuses_plain, 0 },
  { "GlossyPro",	canon_MULTIPASS_MP750_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP750_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP750_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP750_modeuses_PPpro, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP750_modeuses_PPplus, 0 },
  { "Coated",		canon_MULTIPASS_MP750_modeuses_PPplus, 0 },
  { "GlossyFilm",	canon_MULTIPASS_MP750_modeuses_PPplus, 0 },/*untested*/
  { "InkJetHagaki", 	canon_MULTIPASS_MP750_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP750_modeuses_Hagaki, 0 },
  { "DiscCompat",	canon_MULTIPASS_MP750_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP750_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP750_modeuses_TShirt, 0 },
  { "Transparency",	canon_MULTIPASS_MP750_modeuses_Transparency, 0 },
  { "Envelope",		canon_MULTIPASS_MP750_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP750_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP750);

/* ----------------------------------- Canon MP800  ----------------------------------- */
static const char* canon_MULTIPASS_MP800_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/*most photo modes not yet supported */
static const char* canon_MULTIPASS_MP800_modeuses_PPpro[] = {
  "600x600dpi_photo",/*temporary stand-in*/
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

/* only draft available */
static const char* canon_MULTIPASS_MP800_modeuses_PPplus[] = {
  "600x600dpi_photo",/*temporary stand-in*/
  "600x600dpi_photodraft",
  NULL
};

/*most photo modes not yet supported */
static const char* canon_MULTIPASS_MP800_modeuses_PPplusDS[] = {
  "600x600dpi_photo",/*temporary stand-in*/
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP800_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP800_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP800_modeuses_disc[] = {
  "600x600dpi_photo",/*temporary stand-in*/
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP800_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP800_modeuses_PPother[] = {
  "600x600dpi_photo",/*temporary stand-in*/
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP800_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP800_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_MULTIPASS_MP800_modeuses_PPpro, 0 },/*not supported yet*/
 { "PhotopaperPlus",	canon_MULTIPASS_MP800_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP800_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "GlossyPaper",	canon_MULTIPASS_MP800_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "PhotopaperMatte",	canon_MULTIPASS_MP800_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "Coated",		canon_MULTIPASS_MP800_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "InkJetHagaki", 	canon_MULTIPASS_MP800_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP800_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "CD",   	        canon_MULTIPASS_MP800_modeuses_plain, 0 },/*NOTE:temporary replacement*/
 { "DiscCompat",	canon_MULTIPASS_MP800_modeuses_disc, 0 },/*not supported yet*/
 { "DiscOthers",	canon_MULTIPASS_MP800_modeuses_disc, 0 },/*not supported yet*/
 { "TShirt",		canon_MULTIPASS_MP800_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP800_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP800_modeuses_PPother, 0 },/*not supported yet*/
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP800);

/* ----------------------------------- Canon MP810  ----------------------------------- */
static const char* canon_MULTIPASS_MP810_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP810_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP810_modeuses_PPother[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP810_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP810_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_MULTIPASS_MP810_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_MULTIPASS_MP810_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP810_modeuses_PPplusDS, 0 },
 { "GlossyPaper",	canon_MULTIPASS_MP810_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_MULTIPASS_MP810_modeuses_PPmatte, 0 },
 { "Coated",		canon_MULTIPASS_MP810_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_MULTIPASS_MP810_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP810_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "CD",   	        canon_MULTIPASS_MP810_modeuses_plain,  },/*NOTE:option*/
 { "DiscCompat",	canon_MULTIPASS_MP810_modeuses_disc, 0 },
 { "DiscOthers",	canon_MULTIPASS_MP810_modeuses_disc, 0 },
 { "TShirt",		canon_MULTIPASS_MP810_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP810_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP810_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP810);

/* ----------------------------------- Canon MP830  ----------------------------------- */
static const char* canon_MULTIPASS_MP830_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/*most photo modes not yet supported */
static const char* canon_MULTIPASS_MP830_modeuses_PPpro[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

/* only draft available */
static const char* canon_MULTIPASS_MP830_modeuses_PPplus[] = {
  "600x600dpi_photodraft",
  NULL
};

/*most photo modes not yet supported */
static const char* canon_MULTIPASS_MP830_modeuses_PPplusDS[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MP830_modeuses_inkjetHagaki[] = {
  "600x600dpi_photo2",
  "600x600dpi_photodraft2",
  NULL
};

static const char* canon_MULTIPASS_MP830_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MP830_modeuses_disc[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP830_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP830_modeuses_PPother[] = {
  "600x600dpi_photodraft",/*temporary stand-in*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP830_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP830_modeuses_plain, DUPLEX_SUPPORT },
 { "GlossyPro",	        canon_MULTIPASS_MP830_modeuses_PPpro, 0 },/*not supported yet*/
 { "PhotopaperPlus",	canon_MULTIPASS_MP830_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "GlossyPaper",	canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "PhotopaperMatte",	canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "Coated",		canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "InkJetHagaki", 	canon_MULTIPASS_MP830_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP830_modeuses_Hagaki, DUPLEX_SUPPORT },
 { "CD",   	        canon_MULTIPASS_MP830_modeuses_plain, 0 },/*NOTE:temporary replacement*/
 { "DiscCompat",	canon_MULTIPASS_MP830_modeuses_disc, 0 },/*not supported yet*/
 { "DiscOthers",	canon_MULTIPASS_MP830_modeuses_disc, 0 },/*not supported yet*/
 { "TShirt",		canon_MULTIPASS_MP830_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP830_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP830_modeuses_PPother, 0 },/*not supported yet*/
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP830);

/* ----------------------------------- Canon MP900  ----------------------------------- */
static const char* canon_MULTIPASS_MP900_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  /* Mono */
  "600x600dpi_draftmono",
  "600x600dpi_draftmono2",
  NULL
  };

static const char* canon_MULTIPASS_MP900_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photo2", /*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2", /*untested*/
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_coated[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_Hagaki[] = {
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",
  "600x600dpi_draftmono4",/* Mono */
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_disc[] = {
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP900_modeuses_Transparency[] = {
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

/*untested*/
static const char* canon_MULTIPASS_MP900_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP900_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP900_modeuses_plain, 0 },
 { "GlossyPro",	        canon_MULTIPASS_MP900_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_MULTIPASS_MP900_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP900_modeuses_PPplusDS, 0 },
 { "GlossyPaper",	canon_MULTIPASS_MP900_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_MULTIPASS_MP900_modeuses_PPmatte, 0 },
 { "Coated",		canon_MULTIPASS_MP900_modeuses_coated, 0 },
 { "InkJetHagaki", 	canon_MULTIPASS_MP900_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP900_modeuses_Hagaki, 0 },
 { "DiscCompat",	canon_MULTIPASS_MP900_modeuses_disc, 0 },
 { "DiscOthers",	canon_MULTIPASS_MP900_modeuses_disc, 0 },
 { "TShirt",		canon_MULTIPASS_MP900_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP900_modeuses_Hagaki, 0 },
 { "Transparency",	canon_MULTIPASS_MP900_modeuses_Transparency, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP900_modeuses_PPother, 0 }, /*untested*/
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP900);

/* ----------------------------------- Canon MP950  ----------------------------------- */
static const char* canon_MULTIPASS_MP950_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/*highest mode not yet supported */
static const char* canon_MULTIPASS_MP950_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  /* Mono not yet supported */
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  /* Mono not yet supported */
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  "600x600dpi_photodraft",/*untested*/
  /* No mono modes for this media */
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  /* No mono modes for this media */
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_FA[] = {
  "600x600dpi_photohigh4",
  /* Mono not yet supported */
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_disc[] = {
  "600x600dpi_photohigh5",
  "600x600dpi_photo5",
  "600x600dpi_photodraft5",
  NULL
  /* No mono modes for this media */
};

static const char* canon_MULTIPASS_MP950_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_Transparency[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* No mono modes for this media */
  NULL
};

static const char* canon_MULTIPASS_MP950_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  "600x600dpi_photodraft",/*untested*/
  /* No mono modes for this media */
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP950_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP950_modeuses_plain, DUPLEX_SUPPORT },
  { "GlossyPro",	canon_MULTIPASS_MP950_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP950_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP950_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP950_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP950_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP950_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP950_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP950_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_MULTIPASS_MP950_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP950_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP950_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP950_modeuses_Hagaki, 0 },
  { "FineArtPhotoRag",	canon_MULTIPASS_MP950_modeuses_FA, 0 },
  { "FineArtOther",	canon_MULTIPASS_MP950_modeuses_FA, 0 },
  { "Transparency",	canon_MULTIPASS_MP950_modeuses_Transparency, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP950_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP950);

/* ----------------------------------- Canon MP960  ----------------------------------- */
static const char* canon_MULTIPASS_MP960_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/*duplex*/
  "600x600dpi",
  "600x600dpi_draft",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/*highest mode not yet supported */
static const char* canon_MULTIPASS_MP960_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* Mono not yet supported */
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* Mono not yet supported */
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* No mono modes for this media */
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  /* No mono modes for this media */
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  /* No mono modes for this media */
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_FA[] = {
  "600x600dpi_photohigh5",
  /* Mono not yet supported */
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
  /* No mono modes for this media */
};

static const char* canon_MULTIPASS_MP960_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* Note: this media is not supported in Windows driver: taken from MP950 */
static const char* canon_MULTIPASS_MP960_modeuses_Transparency[] = {
  "600x600dpi_high4",
  "600x600dpi_std4",
  /* No mono modes for this media */
  NULL
};

static const char* canon_MULTIPASS_MP960_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  /* No mono modes for this media */
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP960_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP960_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_MULTIPASS_MP960_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_MULTIPASS_MP960_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP960_modeuses_PPplusDS, 0 },
 { "GlossyPaper",	canon_MULTIPASS_MP960_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_MULTIPASS_MP960_modeuses_PPmatte, 0 },
 { "Coated",		canon_MULTIPASS_MP960_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_MULTIPASS_MP960_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP960_modeuses_Hagaki, DUPLEX_SUPPORT }, /* not sure */
 { "DiscCompat",	canon_MULTIPASS_MP960_modeuses_disc, 0 },
 { "DiscOthers",	canon_MULTIPASS_MP960_modeuses_disc, 0 },
 { "TShirt",		canon_MULTIPASS_MP960_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP960_modeuses_Hagaki, 0 },
 { "FineArtPhotoRag",	canon_MULTIPASS_MP960_modeuses_FA, 0 },
 { "FineArtOther",	canon_MULTIPASS_MP960_modeuses_FA, 0 },
 { "Transparency",	canon_MULTIPASS_MP960_modeuses_Transparency, 0 },/*untested*/
 { "PhotopaperOther",	canon_MULTIPASS_MP960_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP960);

/* ----------------------------------- Canon MP970  ----------------------------------- */
/* TODO: mono modes for photo media*/

static const char* canon_MULTIPASS_MP970_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/*duplex*/
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/*highest mode not yet supported */
static const char* canon_MULTIPASS_MP970_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_FA[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP970_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP970_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP970_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "GlossyPro",	canon_MULTIPASS_MP970_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP970_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP970_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP970_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP970_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_MULTIPASS_MP970_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP970_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP970_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP970_modeuses_Hagaki, 0 },
  { "FineArtPhotoRag",	canon_MULTIPASS_MP970_modeuses_FA, 0 },
  { "FineArtOther",	canon_MULTIPASS_MP970_modeuses_FA, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP970_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP970);

/* ----------------------------------- Canon MP980  ----------------------------------- */
/* TODO: mono modes for photo media*/

static const char* canon_MULTIPASS_MP980_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/*duplex*/
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/*photo modes not yet supported */
static const char* canon_MULTIPASS_MP980_modeuses_PPpro[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

/*photo modes not yet supported */
static const char* canon_MULTIPASS_MP980_modeuses_PPplus[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP980_modeuses_PPmatte[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP980_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_MULTIPASS_MP980_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

/*photo modes not yet supported */
static const char* canon_MULTIPASS_MP980_modeuses_FA[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP980_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP980_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP980_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP980_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP980_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MP980_modeuses_PPplus, 0 },/*unsupported*/
  { "PhotoPro2",	canon_MULTIPASS_MP980_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProPlat",	canon_MULTIPASS_MP980_modeuses_PPplus, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_MULTIPASS_MP980_modeuses_PPplus, 0 },/*unsupported*/
  { "GlossyPaper",	canon_MULTIPASS_MP980_modeuses_PPplus, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_MULTIPASS_MP980_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP980_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP980_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP980_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_MULTIPASS_MP980_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP980_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP980_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP980_modeuses_Hagaki, 0 },
  { "FineArtPhotoRag",	canon_MULTIPASS_MP980_modeuses_FA, 0 },/*unsupported*/
  { "FineArtOther",	canon_MULTIPASS_MP980_modeuses_FA, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_MULTIPASS_MP980_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP980);

/* ----------------------------------- Canon MP990  ----------------------------------- */
/* Most photo modes not supported */
static const char* canon_MULTIPASS_MP990_modeuses_plain[] = {
  "600x600dpi_high2",
  "600x600dpi_high",/*duplex*/
  "600x600dpi",
  "300x300dpi",
  NULL
  };

/* unsupported */
static const char* canon_MULTIPASS_MP990_modeuses_PPpro[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

/* unsupported */
static const char* canon_MULTIPASS_MP990_modeuses_PPproPlat[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

/* unsupported */
static const char* canon_MULTIPASS_MP990_modeuses_PPplus[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP990_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP990_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MP990_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

/*unsupported*/
static const char* canon_MULTIPASS_MP990_modeuses_FA[] = {
  "600x600dpi_photo",/*stand-in*/
  NULL
};

static const char* canon_MULTIPASS_MP990_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MP990_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP990_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP990_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MP990_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MP990_modeuses_PPplus, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP990_modeuses_PPpro, 0 },
  { "PhotoProPlat",	canon_MULTIPASS_MP990_modeuses_PPproPlat, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MP990_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP990_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP990_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP990_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP990_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MP990_modeuses_PPplus, 0 },
  { "ProPhotoHagakiP",  canon_MULTIPASS_MP990_modeuses_PPproPlat, 0 },
  { "HagakiA", 	        canon_MULTIPASS_MP990_modeuses_Hagaki, DUPLEX_SUPPORT },/* not sure */
  { "Hagaki", 	        canon_MULTIPASS_MP990_modeuses_Hagaki, DUPLEX_SUPPORT },/* not sure */
  { "DiscCompat",	canon_MULTIPASS_MP990_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP990_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP990_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP990_modeuses_Hagaki, 0 },
  { "FineArtPhotoRag",	canon_MULTIPASS_MP990_modeuses_FA, 0 },
  { "FineArtOther",	canon_MULTIPASS_MP990_modeuses_FA, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP990_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP990);

/* ----------------------------------- Canon MX300  ----------------------------------- */
static const char* canon_MULTIPASS_MX300_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Black */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  "300x300dpi_std2",
  "300x300dpi_draft2",
  NULL
  };

static const char* canon_MULTIPASS_MX300_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX300_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX300_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MX300_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MX300_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  /* Black */
  "600x600dpi_highmono3",
  "600x600dpi_stdmono3",
  "600x600dpi_draftmono3",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MX300_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX300_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX300_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX300_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX300_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPro",	canon_MULTIPASS_MX300_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlus",   canon_MULTIPASS_MX300_modeuses_PPplus, INKSET_COLOR_SUPPORT },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MX300_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX300_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MX300_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX300_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MX300_modeuses_PPmatte, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX300_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX300_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX300_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MX300_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX300);

/* ----------------------------------- Canon MX330  ----------------------------------- */
static const char* canon_MULTIPASS_MX330_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Black */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  "300x300dpi_std2",
  "300x300dpi_draft2",
  NULL
  };

static const char* canon_MULTIPASS_MX330_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX330_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX330_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  /* Black */
  "600x600dpi_highmono3",
  "600x600dpi_stdmono3",
  "600x600dpi_draftmono3",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MX330_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX330_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX330_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX330_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX330_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoPro2",	canon_MULTIPASS_MX330_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_MX330_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX330_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MX330_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX330_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX330_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MX330_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX330_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX330_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX330_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MX330_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX330);

/* ----------------------------------- Canon MX340  ----------------------------------- */
static const char* canon_MULTIPASS_MX340_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  /* Black */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",/*untested*/
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  "300x300dpi_std2",
  "300x300dpi_draft2",/*untested*/
  NULL
  };

static const char* canon_MULTIPASS_MX340_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX340_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX340_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  /* Black */
  "600x600dpi_highmono3",
  "600x600dpi_stdmono3",
  "600x600dpi_draftmono3",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MX340_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX340_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX340_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX340_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoPro2",	canon_MULTIPASS_MX340_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MX340_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX340_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX340_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX340_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX340_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MX340_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX340);

/* ----------------------------------- Canon MX360  ----------------------------------- */
static const char* canon_MULTIPASS_MX360_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  /* Black */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",/*untested*/
  /* Color */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",/*untested*/
  "300x300dpi_std3",
  "300x300dpi_draft3",/*untested*/
  NULL
  };

static const char* canon_MULTIPASS_MX360_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX360_modeuses_PPproPlat[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX360_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  /* Black */
  "600x600dpi_highmono2",
  "600x600dpi_stdmono2",
  "600x600dpi_draftmono2",/*untested*/
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "600x600dpi_draft4",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MX360_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX360_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MX360_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MX360_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotoPlusGloss2",   canon_MULTIPASS_MX360_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MX360_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MX360_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MX360_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MX360_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MX360_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "HagakiA", 	        canon_MULTIPASS_MX360_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "InkJetHagaki", 	canon_MULTIPASS_MX360_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "CanonPhotoHagakiK", canon_MULTIPASS_MX360_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MX360_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MX360_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MX360_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MX360_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MX360);

/* ----------------------------------- Canon MX370  ----------------------------------- */
static const char* canon_MULTIPASS_MX370_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  /* color-only */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "300x300dpi_draft3",
  NULL
};

static const char* canon_MULTIPASS_MX370_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX370_modeuses_PPproPlat[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX370_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MX370_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX370_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX370_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX370_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX370_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",	canon_MULTIPASS_MX370_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX370_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX370_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MX370_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX370_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MX370_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_MX370_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX370_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX370_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX370_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX370_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MX370_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX370);
/* ----------------------------------- Canon MX390  ----------------------------------- */
static const char* canon_MULTIPASS_MX390_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  /* color-only */
  "600x600dpi_high3",
  "600x600dpi_std3",
  "300x300dpi_draft3",
  NULL
  };


static const char* canon_MULTIPASS_MX390_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX390_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX390_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MX390_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX390_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX390_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX390_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_MX390_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MX390_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX390_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX390_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX390_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX390_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_MULTIPASS_MX390_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX390);
/* ----------------------------------- Canon MX420  ----------------------------------- */
static const char* canon_MULTIPASS_MX420_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  /* Black */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",/*untested*/
  "300x300dpi_mono",
  "300x300dpi_draftmono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "600x600dpi_draft2",/*untested*/
  "300x300dpi_std2",
  "300x300dpi_draft2",
  NULL
  };

static const char* canon_MULTIPASS_MX420_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MX420_modeuses_PPproPlat[] = {
  "1200x1200dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

static const char* canon_MULTIPASS_MX420_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* Black */
  "600x600dpi_highmono3",
  "600x600dpi_stdmono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_MULTIPASS_MX420_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX420_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",/*untested*/
  "600x600dpi_photodraft",/*untested*/
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MX420_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MX420_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotoPlusGloss2",   canon_MULTIPASS_MX420_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MX420_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MX420_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MX420_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MX420_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MX420_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "HagakiA", 	        canon_MULTIPASS_MX420_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "InkJetHagaki", 	canon_MULTIPASS_MX420_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "CanonPhotoHagakiK", canon_MULTIPASS_MX420_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MX420_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MX420_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MX420_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MX420_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MX420);

/* ----------------------------------- Canon MX470  ----------------------------------- */

static const char* canon_MULTIPASS_MX470_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high5",/* duplex */
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_MULTIPASS_MX470_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX470_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX470_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MX470_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_MULTIPASS_MX470_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX470_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX470_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_MX470_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",	canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MX470_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiK",canon_MULTIPASS_MX470_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX470_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX470_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX470_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_MULTIPASS_MX470_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX470);

/* ----------------------------------- Canon MX510  ----------------------------------- */
static const char* canon_MULTIPASS_MX510_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high3",/* duplex */
  "600x600dpi",
  "300x300dpi",
  /* color-only */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "300x300dpi_std4",
  NULL
};

static const char* canon_MULTIPASS_MX510_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX510_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX510_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MX510_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX510_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX510_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX510_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",	canon_MULTIPASS_MX510_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MX510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MX510_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MX510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX510_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT },
  { "TShirt",		canon_MULTIPASS_MX510_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX510_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MX510_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX510);

/* ----------------------------------- Canon MX520  ----------------------------------- */
static const char* canon_MULTIPASS_MX520_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high3",/* duplex */
  "600x600dpi",
  "300x300dpi",
  /* color-only */
  "600x600dpi_high4",
  "600x600dpi_std4",
  "300x300dpi_std4",
  NULL
};

static const char* canon_MULTIPASS_MX520_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX520_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX520_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MX520_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX520_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX520_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX520_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",	canon_MULTIPASS_MX520_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",	canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MX520_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX520_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX520_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX520_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX520_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_MX520_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX520);

/* ----------------------------------- Canon MX530  ----------------------------------- */

static const char* canon_MULTIPASS_MX530_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high5",/* duplex */
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_MULTIPASS_MX530_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX530_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX530_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MX530_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_MULTIPASS_MX530_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX530_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX530_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_MX530_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",	canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_MX530_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiK",canon_MULTIPASS_MX530_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_MX530_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_MX530_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX530_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_MULTIPASS_MX530_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX530);

/* ----------------------------------- Canon MX700  ----------------------------------- */
static const char* canon_MULTIPASS_MX700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MX700_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX700_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MX700_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_MULTIPASS_MX700_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MX700_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX700_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MX700_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX700_modeuses_plain, 0 },
  { "GlossyPro",	canon_MULTIPASS_MX700_modeuses_PPpro, 0 },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MX700_modeuses_PPplus, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MX700_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MX700_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MX700_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MX700_modeuses_PPplusDS, 0 },
  { "Coated",		canon_MULTIPASS_MX700_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MX700_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MX700_modeuses_Hagaki, 0 },
  { "TShirt",		canon_MULTIPASS_MX700_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MX700_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MX700_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX700);

/* ----------------------------------- Canon MX710  ----------------------------------- */
static const char* canon_MULTIPASS_MX710_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2", /* duplex */
  "300x300dpi",
  NULL
};

static const char* canon_MULTIPASS_MX710_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode unsupported */
static const char* canon_MULTIPASS_MX710_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX710_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MX710_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MX710_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MX710_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX710_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX710_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX710_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX710_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",	canon_MULTIPASS_MX710_modeuses_PPproPlat, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX710_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MX710_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MX710_modeuses_PPplusG2, 0 },
  { "Coated",		canon_MULTIPASS_MX710_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_MULTIPASS_MX710_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MX710_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX710_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MX710_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "TShirt",		canon_MULTIPASS_MX710_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MX710_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MX710_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX710);

/* ----------------------------------- Canon MX720  ----------------------------------- */
static const char* canon_PIXMA_MX720_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/*duplex*/
  "300x300dpi",
  NULL
};

static const char* canon_PIXMA_MX720_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode still unsupported */
static const char* canon_PIXMA_MX720_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MX720_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MX720_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MX720_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MX720_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_MX720_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MX720_modeuses[] = {
  { "Plain",            canon_PIXMA_MX720_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MX720_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MX720_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_MX720_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MX720_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MX720_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MX720_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MX720_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MX720_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MX720_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MX720_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MX720_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "TShirt",		canon_PIXMA_MX720_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MX720_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MX720_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MX720);

/* ----------------------------------- Canon MX850  ----------------------------------- */
static const char* canon_MULTIPASS_MX850_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_MULTIPASS_MX850_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MX850_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MX850_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX850_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MX850_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char* canon_MULTIPASS_MX850_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MX850_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char* canon_MULTIPASS_MX850_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX850_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MX850_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX850_modeuses_plain, DUPLEX_SUPPORT },
  { "PhotoPlusGloss2",	canon_MULTIPASS_MX850_modeuses_PPplus, 0 },
  { "GlossyPro",	canon_MULTIPASS_MX850_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MX850_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MX850_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MX850_modeuses_PPplusDS, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX850_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MX850_modeuses_PPmatte, 0 },
  { "Coated",	        canon_MULTIPASS_MX850_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MX850_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MX850_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_MULTIPASS_MX850_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MX850_modeuses_disc, 0 },
  { "TShirt",	        canon_MULTIPASS_MX850_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MX850_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MX850_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX850);

/* ----------------------------------- Canon MX860  ----------------------------------- */
static const char* canon_MULTIPASS_MX860_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_MULTIPASS_MX860_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MX860_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX860_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MX860_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MX860_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MX860_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX860_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX860_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX860_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX860_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MX860_modeuses_PPpro, 0 },
  { "PhotoProPlat",     canon_MULTIPASS_MX860_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MX860_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MX860_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX860_modeuses_PPplusG2, 0 },
  { "Coated",		canon_MULTIPASS_MX860_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MX860_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MX860_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "TShirt",		canon_MULTIPASS_MX860_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MX860_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MX860_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX860);

/* ----------------------------------- Canon MX870  ----------------------------------- */
static const char* canon_MULTIPASS_MX870_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_MULTIPASS_MX870_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MX870_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX870_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MX870_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MX870_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MX870_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX870_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX870_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX870_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX870_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MX870_modeuses_PPpro, 0 },
  { "PhotoProPlat",     canon_MULTIPASS_MX870_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MX870_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MX870_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX870_modeuses_PPplusG2, 0 },
  { "Coated",		canon_MULTIPASS_MX870_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_MULTIPASS_MX870_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MX870_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX870_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MX870_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "TShirt",		canon_MULTIPASS_MX870_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MX870_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MX870_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX870);

/* ----------------------------------- Canon MX880  ----------------------------------- */
static const char* canon_MULTIPASS_MX880_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_MULTIPASS_MX880_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MX880_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX880_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_MULTIPASS_MX880_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_MULTIPASS_MX880_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_MX880_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MX880_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX880_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX880_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX880_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_MULTIPASS_MX880_modeuses_PPproPlat, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX880_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MX880_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MX880_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MX880_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_MULTIPASS_MX880_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_MX880_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_MULTIPASS_MX880_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MX880_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "TShirt",		canon_MULTIPASS_MX880_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MX880_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MX880_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX880);

/* ----------------------------------- Canon MX920  ----------------------------------- */
static const char* canon_PIXMA_MX920_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/*duplex*/
  "300x300dpi",
  NULL
};

static const char* canon_PIXMA_MX920_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode still unsupported */
static const char* canon_PIXMA_MX920_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MX920_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MX920_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MX920_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MX920_modeuses_disc[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MX920_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_MX920_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MX920_modeuses[] = {
  { "Plain",            canon_PIXMA_MX920_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MX920_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MX920_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_MX920_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MX920_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MX920_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MX920_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MX920_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MX920_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MX920_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MX920_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MX920_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MX920_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MX920_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MX920_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MX920_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MX920_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MX920);

/* ----------------------------------- Canon MX7600  ----------------------------------- */
static const char* canon_MULTIPASS_MX7600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_MULTIPASS_MX7600_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode not yet supported */
static const char* canon_MULTIPASS_MX7600_modeuses_PPsemiGloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MX7600_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_MULTIPASS_MX7600_modeuses_FA[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MX7600_modeuses[] = {
  { "Plain",            canon_MULTIPASS_MX7600_modeuses_plain, DUPLEX_SUPPORT },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MX7600_modeuses_PPplusG2, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MX7600_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MX7600_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_MULTIPASS_MX7600_modeuses_PPsemiGloss, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MX7600_modeuses_PPplusG2, 0 },
  { "FineArtPhotoRag", 	canon_MULTIPASS_MX7600_modeuses_FA, 0 },
  { "FineArtOther", 	canon_MULTIPASS_MX7600_modeuses_FA, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MX7600_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "Envelope",		canon_MULTIPASS_MX7600_modeuses_Hagaki, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MX7600);

/* ----------------------------------- Canon E400  ----------------------------------- */

static const char* canon_MULTIPASS_E400_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const char* canon_MULTIPASS_E400_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E400_modeuses_PPglossy[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_E400_modeuses[] = {
  { "Plain",            canon_MULTIPASS_E400_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "PhotoPlusGloss2",  canon_MULTIPASS_E400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",	canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_E400_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiK",canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_E400_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "TShirt",		canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_E400_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "PhotopaperOther",	canon_MULTIPASS_E400_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_E400);

/* ----------------------------------- Canon E480  ----------------------------------- */
static const char* canon_MULTIPASS_E480_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* color-only */
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const char* canon_MULTIPASS_E480_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E480_modeuses_PP[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E480_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_E480_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_MULTIPASS_E480_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_E480_modeuses[] = {
  { "Plain",            canon_MULTIPASS_E480_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_E480_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",	canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_E480_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiK",canon_MULTIPASS_E480_modeuses_PP, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_E480_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "TShirt",		canon_MULTIPASS_E480_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_E480_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "PhotopaperOther",	canon_MULTIPASS_E480_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_E480);

/* ----------------------------------- Canon E500  ----------------------------------- */
static const char* canon_MULTIPASS_E500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* color-only */
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const char* canon_MULTIPASS_E500_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E500_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E500_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_E500_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_E500_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_E500_modeuses[] = {
  { "Plain",            canon_MULTIPASS_E500_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_E500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_E500_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_E500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_E500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_E500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_E500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_E500_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_E500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_E500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_E500_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_E500_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_E500_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_E500_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_E500);

/* ----------------------------------- Canon E510  ----------------------------------- */
static const char* canon_MULTIPASS_E510_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* color-only */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

static const char* canon_MULTIPASS_E510_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E510_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E510_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_E510_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_E510_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_E510_modeuses[] = {
  { "Plain",            canon_MULTIPASS_E510_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_E510_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_E510_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_MULTIPASS_E510_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_E510_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_E510_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_E510_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_E510_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_E510);

/* ----------------------------------- Canon E560  ----------------------------------- */
static const char* canon_MULTIPASS_E560_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* color-only */
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const char* canon_MULTIPASS_E560_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E560_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_E560_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_MULTIPASS_E560_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_MULTIPASS_E560_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_E560_modeuses[] = {
  { "Plain",            canon_MULTIPASS_E560_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL},
  { "PhotoPlusGloss2",  canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_MULTIPASS_E560_modeuses_PPproPlat, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",	canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_MULTIPASS_E560_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiK",canon_MULTIPASS_E560_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_MULTIPASS_E560_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_MULTIPASS_E560_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_MULTIPASS_E560_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotopaperOther",	canon_MULTIPASS_E560_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_MULTIPASS_E560);

/* ----------------------------------- Canon P200  ----------------------------------- */

static const char* canon_PIXMA_P200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  NULL
};

static const char* canon_PIXMA_P200_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_P200_modeuses_PPglossy[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_P200_modeuses[] = {
  { "Plain",            canon_PIXMA_P200_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "PhotoPlusGloss2",  canon_PIXMA_P200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_P200_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_P200_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "TShirt",		canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_P200_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT },
  { "PhotopaperOther",	canon_PIXMA_P200_modeuses_PPglossy, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_P200);

/* ----------------------------------- Canon MG2100  ----------------------------------- */
static const char* canon_PIXMA_MG2100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  /* Mono */
  "600x600dpi_monohigh",
  "600x600dpi_mono",
  "300x300dpi_mono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "300x300dpi_std2",
  NULL
  };

static const char* canon_PIXMA_MG2100_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2100_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* Mono */
  "600x600dpi_highmono3",
  "600x600dpi_mono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_PIXMA_MG2100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG2100_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG2100_modeuses[] = {
  { "Plain",            canon_PIXMA_MG2100_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG2100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG2100_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG2100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG2100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_MG2100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG2100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG2100_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_MG2100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_MG2100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG2100_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_MG2100_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG2100_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG2100_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG2100);

/* ----------------------------------- Canon MG2200  ----------------------------------- */
static const char* canon_PIXMA_MG2200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  /* Mono */
  "600x600dpi_monohigh",
  "600x600dpi_mono",
  "300x300dpi_mono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "300x300dpi_std2",
  NULL
  };

static const char* canon_PIXMA_MG2200_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2200_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2200_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* Mono */
  "600x600dpi_highmono3",
  "600x600dpi_mono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_PIXMA_MG2200_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG2200_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG2200_modeuses[] = {
  { "Plain",            canon_PIXMA_MG2200_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG2200_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG2200_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_MG2200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG2200_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_MG2200_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG2200_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG2200_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG2200);

/* ----------------------------------- Canon MG2400  ----------------------------------- */
static const char* canon_PIXMA_MG2400_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG2400_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2400_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2400_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG2400_modeuses_TShirt[] = {
  "600x600dpi_photo",
  NULL
  };

static const char* canon_PIXMA_MG2400_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG2400_modeuses[] = {
  { "Plain",            canon_PIXMA_MG2400_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG2400_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG2400_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_MG2400_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG2400_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_MG2400_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG2400_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG2400_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG2400);

/* ----------------------------------- Canon MG2900  ----------------------------------- */
static const char* canon_PIXMA_MG2900_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG2900_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2900_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG2900_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG2900_modeuses_TShirt[] = {
  "600x600dpi_photo",
  NULL
  };

static const char* canon_PIXMA_MG2900_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG2900_modeuses[] = {
  { "Plain",            canon_PIXMA_MG2900_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG2900_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",	canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG2900_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiK",canon_PIXMA_MG2900_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG2900_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_MG2900_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG2900_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG2900_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG2900);

/* ----------------------------------- Canon MG3100  ----------------------------------- */
static const char* canon_PIXMA_MG3100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high5",/* duplex */
  "600x600dpi",
  "300x300dpi",
  /* Mono */
  "600x600dpi_monohigh",
  "600x600dpi_mono",
  "300x300dpi_mono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "300x300dpi_std2",
  NULL
  };

static const char* canon_PIXMA_MG3100_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3100_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* Mono */
  "600x600dpi_highmono3",
  "600x600dpi_mono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_PIXMA_MG3100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG3100_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG3100_modeuses[] = {
  { "Plain",            canon_PIXMA_MG3100_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG3100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG3100_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG3100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG3100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_MG3100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG3100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG3100_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG3100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_MG3100_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG3100_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT },
  { "TShirt",		canon_PIXMA_MG3100_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG3100_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG3100_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG3100);

/* ----------------------------------- Canon MG3200  ----------------------------------- */
static const char* canon_PIXMA_MG3200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high5",/* duplex */
  "600x600dpi",
  "300x300dpi",
  /* Mono */
  "600x600dpi_monohigh",
  "600x600dpi_mono",
  "300x300dpi_mono",
  /* Color */
  "600x600dpi_high2",
  "600x600dpi_std2",
  "300x300dpi_std2",
  NULL
  };

static const char* canon_PIXMA_MG3200_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3200_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3200_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  /* Mono */
  "600x600dpi_highmono3",
  "600x600dpi_mono3",
  /* Color */
  "600x600dpi_high4",
  "600x600dpi_std4",
  NULL
};

static const char* canon_PIXMA_MG3200_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG3200_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG3200_modeuses[] = {
  { "Plain",            canon_PIXMA_MG3200_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG3200_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG3200_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_MG3200_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG3200_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT },
  { "TShirt",		canon_PIXMA_MG3200_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG3200_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG3200_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG3200);

/* ----------------------------------- Canon MG3500  ----------------------------------- */
/* same as MG3100 but try to use inktypes to control use of inks in inksets */
static const char* canon_PIXMA_MG3500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high5",/* duplex */
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG3500_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3500_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3500_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MG3500_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_MG3500_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG3500_modeuses[] = {
  { "Plain",            canon_PIXMA_MG3500_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG3500_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaper",	canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG3500_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "CanonPhotoHagakiK",canon_PIXMA_MG3500_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG3500_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_MG3500_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG3500_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG3500_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG3500);

/* ----------------------------------- Canon MG3600  ----------------------------------- */
static const char* canon_PIXMA_MG3600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high5",/* color */
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG3600_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3600_modeuses_PPpro[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG3600_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MG3600_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

static const char* canon_PIXMA_MG3600_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG3600_modeuses[] = {
  { "Plain",            canon_PIXMA_MG3600_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProPlat",     canon_PIXMA_MG3600_modeuses_PPpro, INKSET_COLOR_SUPPORT },
  { "PhotoProLuster",   canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotoProSemiGloss",canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "PhotopaperMatte",	canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "GlossyPaperStandard",canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "Coated",		canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "HagakiA", 	        canon_PIXMA_MG3600_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkJetHagaki", 	canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "InkJetHagakiaddr", canon_PIXMA_MG3600_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "InkjetPhotoHagakiK",canon_PIXMA_MG3600_modeuses_PPplusG2, INKSET_COLOR_SUPPORT },
  { "InkjetPhotoHagakiKaddr",canon_PIXMA_MG3600_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "Hagaki", 	        canon_PIXMA_MG3600_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
  { "TShirt",		canon_PIXMA_MG3600_modeuses_TShirt, INKSET_COLOR_SUPPORT },
  { "Envelope",		canon_PIXMA_MG3600_modeuses_Hagaki, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL }, 
  { "PhotopaperOther",	canon_PIXMA_MG3600_modeuses_PPother, INKSET_COLOR_SUPPORT },
};

DECLARE_MODEUSES(canon_PIXMA_MG3600);

/* ----------------------------------- Canon MG5100  ----------------------------------- */
static const char* canon_PIXMA_MG5100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG5100_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode still unsupported */
static const char* canon_PIXMA_MG5100_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MG5100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MG5100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG5100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG5100_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG5100_modeuses[] = {
  { "Plain",            canon_PIXMA_MG5100_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG5100_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG5100_modeuses_PPpro, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG5100_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MG5100_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG5100_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MG5100_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG5100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG5100_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG5100_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MG5100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "TShirt",		canon_PIXMA_MG5100_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG5100_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MG5100_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG5100);

/* ----------------------------------- Canon MG5200  ----------------------------------- */
static const char* canon_PIXMA_MG5200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",/*untested*/
  NULL
  };

static const char* canon_PIXMA_MG5200_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode still unsupported */
static const char* canon_PIXMA_MG5200_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5200_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MG5200_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MG5200_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG5200_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG5200_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG5200_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG5200_modeuses[] = {
  { "Plain",            canon_PIXMA_MG5200_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG5200_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG5200_modeuses_PPpro, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG5200_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MG5200_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG5200_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MG5200_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG5200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG5200_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG5200_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MG5200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG5200_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG5200_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG5200_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG5200_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MG5200_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG5200);

/* ----------------------------------- Canon MG5300  ----------------------------------- */
static const char* canon_PIXMA_MG5300_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/*duplex*/
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG5300_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode still unsupported */
static const char* canon_PIXMA_MG5300_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5300_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MG5300_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG5300_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG5300_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG5300_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG5300_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG5300_modeuses[] = {
  { "Plain",            canon_PIXMA_MG5300_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG5300_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG5300_modeuses_PPpro, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG5300_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MG5300_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG5300_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MG5300_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG5300_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG5300_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG5300_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MG5300_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG5300_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG5300_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG5300_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG5300_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MG5300_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG5300);

/* ----------------------------------- Canon MG5400  ----------------------------------- */
static const char* canon_PIXMA_MG5400_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/*duplex*/
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG5400_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

/* highest mode still unsupported */
static const char* canon_PIXMA_MG5400_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5400_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MG5400_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG5400_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG5400_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG5400_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG5400_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG5400_modeuses[] = {
  { "Plain",            canon_PIXMA_MG5400_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG5400_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG5400_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_MG5400_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG5400_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MG5400_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG5400_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MG5400_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG5400_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG5400_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG5400_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MG5400_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG5400_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG5400_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG5400_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG5400_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MG5400_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG5400);

/* ----------------------------------- Canon MG5500  ----------------------------------- */
static const char* canon_PIXMA_MG5500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG5500_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5500_modeuses_PPpro[] = {
  /*ud1 not supported */
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5500_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MG5500_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG5500_modeuses_TShirt[] = {
  "600x600dpi_photohigh2",
  NULL
};

static const char* canon_PIXMA_MG5500_modeuses_PPother[] = {
  "600x600dpi_photohigh", /* experimental */
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG5500_modeuses[] = {
  { "Plain",            canon_PIXMA_MG5500_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG5500_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG5500_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_MG5500_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG5500_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MG5500_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG5500_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MG5500_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG5500_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_PIXMA_MG5500_modeuses_PPplusG2, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG5500_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MG5500_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_MG5500_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG5500_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MG5500_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG5500);

/* ----------------------------------- Canon MG5600  ----------------------------------- */
static const char* canon_PIXMA_MG5600_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
  };

static const char* canon_PIXMA_MG5600_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5600_modeuses_PPpro[] = {
  /*ud1 not supported */
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5600_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_MG5600_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG5600_modeuses_TShirt[] = {
  "600x600dpi_photohigh2",
  NULL
};

static const char* canon_PIXMA_MG5600_modeuses_PPother[] = {
  "600x600dpi_photohigh", /* experimental */
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG5600_modeuses[] = {
  { "Plain",            canon_PIXMA_MG5600_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG5600_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG5600_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_MG5600_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG5600_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MG5600_modeuses_PPmatte, 0 },
  { "GlossyPaperStandard",	canon_PIXMA_MG5600_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MG5600_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG5600_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_PIXMA_MG5600_modeuses_PPplusG2, 0 },
  { "InkjetPhotoHagakiK",canon_PIXMA_MG5600_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_MG5600_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_MG5600_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG5600_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MG5600_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG5600);

/* ----------------------------------- Canon MG5700  ----------------------------------- */
static const char* canon_PIXMA_MG5700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "600x600dpi_draft",
  NULL
  };

static const char* canon_PIXMA_MG5700_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5700_modeuses_PPpro[] = {
  /*ud1 not supported */
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5700_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MG5700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG5700_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_MG5700_modeuses_PPother[] = {
  "600x600dpi_photohigh",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG5700_modeuses[] = {
  { "Plain",            canon_PIXMA_MG5700_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG5700_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG5700_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_MG5700_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG5700_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_MG5700_modeuses_PPplusG2, 0 },
  { "GlossyPaperStandard",canon_PIXMA_MG5700_modeuses_PPplusG2, 0 },
  { "Coated",		canon_PIXMA_MG5700_modeuses_PPplusG2, 0 },
  { "InkJetHagakiaddr", canon_PIXMA_MG5700_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_PIXMA_MG5700_modeuses_inkjetHagaki, 0 },
  { "InkjetPhotoHagakiKaddr",canon_PIXMA_MG5700_modeuses_Hagaki, 0 },
  { "InkjetPhotoHagakiK",canon_PIXMA_MG5700_modeuses_PPplusG2, 0 },
  { "HagakiA", 	        canon_PIXMA_MG5700_modeuses_Hagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_MG5700_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_MG5700_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG5700_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_PIXMA_MG5700_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG5700);

/* ----------------------------------- Canon MG6100  ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG6100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "300x300dpi",
  NULL
  };

/* unsupported*/
static const char* canon_PIXMA_MG6100_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6100_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG6100_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MG6100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG6100_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG6100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6100_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6100_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG6100_modeuses[] = {
  { "Plain",            canon_PIXMA_MG6100_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG6100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProPlat",     canon_PIXMA_MG6100_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_PIXMA_MG6100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_PIXMA_MG6100_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG6100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Coated",		canon_PIXMA_MG6100_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG6100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG6100_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG6100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Hagaki", 	        canon_PIXMA_MG6100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG6100_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG6100_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG6100_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG6100_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_MG6100_modeuses_photorag, 0 },/*unsupported*/
  { "FineArtOther",     canon_PIXMA_MG6100_modeuses_photorag, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_PIXMA_MG6100_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG6100);

/* ----------------------------------- Canon MG6200  ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG6200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
  };

/* unsupported*/
static const char* canon_PIXMA_MG6200_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6200_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6200_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG6200_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG6200_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG6200_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG6200_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6200_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6200_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG6200_modeuses[] = {
  { "Plain",            canon_PIXMA_MG6200_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG6200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProPlat",     canon_PIXMA_MG6200_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_PIXMA_MG6200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_PIXMA_MG6200_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG6200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Coated",		canon_PIXMA_MG6200_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG6200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG6200_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG6200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Hagaki", 	        canon_PIXMA_MG6200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG6200_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG6200_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG6200_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG6200_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_MG6200_modeuses_photorag, 0 },/*unsupported*/
  { "FineArtOther",     canon_PIXMA_MG6200_modeuses_photorag, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_PIXMA_MG6200_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG6200);

/* ----------------------------------- Canon MG6300  ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG6300_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
  };

/* unsupported*/
static const char* canon_PIXMA_MG6300_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6300_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6300_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG6300_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG6300_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG6300_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG6300_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6300_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6300_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG6300_modeuses[] = {
  { "Plain",            canon_PIXMA_MG6300_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG6300_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProPlat",     canon_PIXMA_MG6300_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProLuster",   canon_PIXMA_MG6300_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_PIXMA_MG6300_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_PIXMA_MG6300_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG6300_modeuses_PPmatte, 0 },/*unsupported*/
  { "Coated",		canon_PIXMA_MG6300_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG6300_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG6300_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG6300_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Hagaki", 	        canon_PIXMA_MG6300_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG6300_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG6300_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG6300_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG6300_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_MG6300_modeuses_photorag, 0 },/*unsupported*/
  { "FineArtOther",     canon_PIXMA_MG6300_modeuses_photorag, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_PIXMA_MG6300_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG6300);

/* ----------------------------------- Canon MG6500  ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG6500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
  };

/* unsupported*/
static const char* canon_PIXMA_MG6500_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6500_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6500_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG6500_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG6500_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG6500_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG6500_modeuses_TShirt[] = {
  "600x600dpi_photo",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6500_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6500_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG6500_modeuses[] = {
  { "Plain",            canon_PIXMA_MG6500_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG6500_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProPlat",     canon_PIXMA_MG6500_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProLuster",   canon_PIXMA_MG6500_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_PIXMA_MG6500_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_PIXMA_MG6500_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG6500_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Coated",		canon_PIXMA_MG6500_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG6500_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG6500_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG6500_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Hagaki", 	        canon_PIXMA_MG6500_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG6500_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG6500_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG6500_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG6500_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_MG6500_modeuses_photorag, 0 },/*unsupported*/
  { "FineArtOther",     canon_PIXMA_MG6500_modeuses_photorag, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_PIXMA_MG6500_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG6500);

/* ----------------------------------- Canon MG6700  ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG6700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
  };

/* unsupported*/
static const char* canon_PIXMA_MG6700_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6700_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6700_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG6700_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG6700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG6700_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG6700_modeuses_TShirt[] = {
  "600x600dpi_photo",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG6700_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG6700_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG6700_modeuses[] = {
  { "Plain",            canon_PIXMA_MG6700_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG6700_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProPlat",     canon_PIXMA_MG6700_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProLuster",   canon_PIXMA_MG6700_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_PIXMA_MG6700_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_PIXMA_MG6700_modeuses_PPmatte, 0 },
  { "GlossyPaperStandard",	canon_PIXMA_MG6700_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Coated",		canon_PIXMA_MG6700_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG6700_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG6700_modeuses_inkjetHagaki, 0 },
  { "InkjetPhotoHagakiK",canon_PIXMA_MG6700_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Hagaki", 	        canon_PIXMA_MG6700_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG6700_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG6700_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG6700_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG6700_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_MG6700_modeuses_photorag, 0 },/*unsupported*/
  { "FineArtOther",     canon_PIXMA_MG6700_modeuses_photorag, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_PIXMA_MG6700_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG6700);

/* ----------------------------------- Canon MG7700  ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG7700_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "600x600dpi_draft",
  NULL
  };

static const char* canon_PIXMA_MG7700_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG7700_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  NULL
};

static const char* canon_PIXMA_MG7700_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG7700_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG7700_modeuses_TShirt[] = {
  "600x600dpi_photohigh",
  NULL
};

/* media using modes requiring H ink are commented out */
static const canon_modeuse_t canon_PIXMA_MG7700_modeuses[] = {
  { "Plain",            canon_PIXMA_MG7700_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  /*  { "PhotoPlusGloss2",  canon_PIXMA_MG7700_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_PIXMA_MG7700_modeuses_PPpro, 0 },
  { "PhotoProLuster",   canon_PIXMA_MG7700_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_MG7700_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_MG7700_modeuses_PPplusG2, 0 },*/
  { "PhotopaperMatte",	canon_PIXMA_MG7700_modeuses_PPmatte, 0 },
  { "Coated",		canon_PIXMA_MG7700_modeuses_PPmatte, 0 },
  { "InkJetHagakiaddr", canon_PIXMA_MG7700_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG7700_modeuses_inkjetHagaki, 0 },
  { "InkjetPhotoHagakiKaddr",canon_PIXMA_MG7700_modeuses_Hagaki, DUPLEX_SUPPORT },
  /*{ "InkjetPhotoHagakiK",canon_PIXMA_MG7700_modeuses_PPplusG2, 0 },*/
  { "HagakiA", 	        canon_PIXMA_MG7700_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "Hagaki", 	        canon_PIXMA_MG7700_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG7700_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG7700_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG7700_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG7700_modeuses_Hagaki, 0 }, 
  /*{ "FineArtPhotoRag",  canon_PIXMA_MG7700_modeuses_photorag, 0 },
  { "FineArtOther",     canon_PIXMA_MG7700_modeuses_photorag, 0 },
  { "PhotopaperOther",	canon_PIXMA_MG7700_modeuses_PPother, 0 },*/
};

DECLARE_MODEUSES(canon_PIXMA_MG7700);

/* ----------------------------------- Canon MG8100 ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG8100_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG8100_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG8100_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG8100_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG8100_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG8100_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG8100_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG8100_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG8100_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG8100_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG8100_modeuses[] = {
  { "Plain",            canon_PIXMA_MG8100_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG8100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProPlat",     canon_PIXMA_MG8100_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_PIXMA_MG8100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_PIXMA_MG8100_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG8100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Coated",		canon_PIXMA_MG8100_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG8100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG8100_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG8100_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Hagaki", 	        canon_PIXMA_MG8100_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG8100_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG8100_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG8100_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG8100_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_MG8100_modeuses_photorag, 0 },/*unsupported*/
  { "FineArtOther",     canon_PIXMA_MG8100_modeuses_photorag, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_PIXMA_MG8100_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG8100);

/* ----------------------------------- Canon MG8200 ----------------------------------- */
/* most photo modes use gray ink which is unsupported */
/* TODO: mono modes for photo media */
static const char* canon_PIXMA_MG8200_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi_high2",/* duplex */
  "600x600dpi",
  "600x600dpi_std2",/* duplex */
  "300x300dpi",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG8200_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG8200_modeuses_PPpro[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG8200_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_MG8200_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_MG8200_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  NULL
};

static const char* canon_PIXMA_MG8200_modeuses_disc[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_MG8200_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

/* unsupported*/
static const char* canon_PIXMA_MG8200_modeuses_photorag[] = {
  "600x600dpi_photohigh",/*stand-in*/
  "600x600dpi_photo",/*stand-in*/
  "600x600dpi_photo2",/*stand-in*/
  NULL
};

static const char* canon_PIXMA_MG8200_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
};

static const canon_modeuse_t canon_PIXMA_MG8200_modeuses[] = {
  { "Plain",            canon_PIXMA_MG8200_modeuses_plain, DUPLEX_SUPPORT | DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",  canon_PIXMA_MG8200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProPlat",     canon_PIXMA_MG8200_modeuses_PPpro, 0 },/*unsupported*/
  { "PhotoProSemiGloss",canon_PIXMA_MG8200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotopaperMatte",	canon_PIXMA_MG8200_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_MG8200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Coated",		canon_PIXMA_MG8200_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_MG8200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "InkJetHagaki", 	canon_PIXMA_MG8200_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_MG8200_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Hagaki", 	        canon_PIXMA_MG8200_modeuses_Hagaki, DUPLEX_SUPPORT },
  { "DiscCompat",	canon_PIXMA_MG8200_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_MG8200_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_MG8200_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_MG8200_modeuses_Hagaki, 0 }, 
  { "FineArtPhotoRag",  canon_PIXMA_MG8200_modeuses_photorag, 0 },/*unsupported*/
  { "FineArtOther",     canon_PIXMA_MG8200_modeuses_photorag, 0 },/*unsupported*/
  { "PhotopaperOther",	canon_PIXMA_MG8200_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_MG8200);

/* ----------------------------------- Canon Pro9000 ----------------------------------- */

static const char* canon_PIXMA_Pro9000_modeuses_plain[] = {
  "600x600dpi_high2",/* exp */
  "600x600dpi",/* exp */
  "600x600dpi_std2",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  /* Mono */
  "600x600dpi_highmono",/* exp */
  "600x600dpi_mono",/* exp */
  "600x600dpi_draftmono",
  "600x600dpi_draftmono2",
  NULL
  };

/* highest mode not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9000_modeuses_PPplusG2[] = {
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

/* highest mode not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9000_modeuses_PPplus[] = {
  "600x600dpi_photo",
  "600x600dpi_photodraft2",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  "600x600dpi_photomonodraft",
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_PPgloss[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photodraft",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomonodraft",
  NULL
};

/* highest mode not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9000_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomonomed",
  "600x600dpi_photomono",
  NULL
};

/* highest mode not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9000_modeuses_PPproPlat[] = {
  "600x600dpi_photohigh",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomonomed",
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_inkjetHagaki[] = {
  "600x600dpi_photomed2",
  "600x600dpi_photomed",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_Hagaki[] = {
  "600x600dpi_high3",/* exp */
  "600x600dpi_std3",/* exp */ /*Mono High*/
  "600x600dpi_std4",/* exp */ /* bw=2 for mono */
  "600x600dpi_draft3",/* bw=2 for mono */
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo2",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_board[] = {
  "600x600dpi_photohigh5",
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_photorag[] = {
  "600x600dpi_photohigh5",
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_TShirt[] = {
  "600x600dpi_tshirt", /* bw=2 for mono */
  NULL
};

static const char* canon_PIXMA_Pro9000_modeuses_PPother[] = {
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

static const canon_modeuse_t canon_PIXMA_Pro9000_modeuses[] = {
 { "Plain",             canon_PIXMA_Pro9000_modeuses_plain, 0 },
 { "PhotopaperPlus",    canon_PIXMA_Pro9000_modeuses_PPplus, 0 },
 { "PhotoPlusGloss2",   canon_PIXMA_Pro9000_modeuses_PPplusG2, 0 },
 { "GlossyPaper",	canon_PIXMA_Pro9000_modeuses_PPgloss, 0 },
 { "PhotoProSemiGloss", canon_PIXMA_Pro9000_modeuses_PPplusG2, 0 },
 { "GlossyPro",         canon_PIXMA_Pro9000_modeuses_PPpro, 0 },
 { "PhotoPro2",  	canon_PIXMA_Pro9000_modeuses_PPpro, 0 },
 { "PhotoProPlat",	canon_PIXMA_Pro9000_modeuses_PPproPlat, 0 },
 { "PhotopaperMatte",	canon_PIXMA_Pro9000_modeuses_PPmatte, 0 },
 { "Coated",		canon_PIXMA_Pro9000_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_Pro9000_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_Pro9000_modeuses_Hagaki, 0 },
 { "DiscCompat",	canon_PIXMA_Pro9000_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_Pro9000_modeuses_disc, 0 },
 { "Boardpaper",	canon_PIXMA_Pro9000_modeuses_board, 0 },
 { "Canvas",	        canon_PIXMA_Pro9000_modeuses_board, 0 },
 { "FineArtPhotoRag",   canon_PIXMA_Pro9000_modeuses_photorag, 0 },
 { "FineArtOther",      canon_PIXMA_Pro9000_modeuses_board, 0 },
 { "FineArtPremiumMatte",canon_PIXMA_Pro9000_modeuses_board, 0 },
 { "FineArtMuseumEtching",canon_PIXMA_Pro9000_modeuses_photorag, 0 },
 { "TShirt",		canon_PIXMA_Pro9000_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_Pro9000_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_Pro9000_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_Pro9000);

/* ----------------------------------- Canon Pro9000mk2 ----------------------------------- */

static const char* canon_PIXMA_Pro9000mk2_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  "600x600dpi_draft2",
  /* Mono */
  "600x600dpi_highmono",
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  "600x600dpi_draftmono2",
  NULL
  };

/* highest mode not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9000mk2_modeuses_PPplusG2[] = {
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000mk2_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photodraft",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomonodraft",
  NULL
};

/* highest mode not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9000mk2_modeuses_PPpro2[] = {
  "600x600dpi_photomed",
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomonomed",/*untested: quality setting uncertain*/
  "600x600dpi_photomono",
  NULL
};

/* highest mode not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9000mk2_modeuses_PPproPlat[] = {
  "600x600dpi_photomed",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomonomed",
  NULL
};

static const char* canon_PIXMA_Pro9000mk2_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000mk2_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000mk2_modeuses_Hagaki[] = {
  "600x600dpi_high2", /* bw=2 for mono */
  "600x600dpi_std2", /* bw=2 for mono */
  "600x600dpi_draft2",/* bw=2 for mono */
  /* Mono */
  "600x600dpi_high3",
  NULL
};

static const char* canon_PIXMA_Pro9000mk2_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
};

static const char* canon_PIXMA_Pro9000mk2_modeuses_board[] = {
  "600x600dpi_photohigh5",
  /* Mono */
  "600x600dpi_photomonohigh",
  NULL
  };

static const char* canon_PIXMA_Pro9000mk2_modeuses_photorag[] = {
  "600x600dpi_photohigh5",
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomonohigh",
  "600x600dpi_photomono",
  NULL
  };

static const char* canon_PIXMA_Pro9000mk2_modeuses_TShirt[] = {
  "600x600dpi_tshirt", /* bw=2 for mono */
  NULL
};

static const char* canon_PIXMA_Pro9000mk2_modeuses_PPother[] = {
  "600x600dpi_photo",
  /* Mono */
  "600x600dpi_photomono",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_Pro9000mk2_modeuses[] = {
 { "Plain",             canon_PIXMA_Pro9000mk2_modeuses_plain, 0 },
 { "PhotoPlusGloss2",   canon_PIXMA_Pro9000mk2_modeuses_PPplusG2, 0 },
 { "GlossyPaper",	canon_PIXMA_Pro9000mk2_modeuses_PPgloss, 0 },
 { "PhotoProSemiGloss", canon_PIXMA_Pro9000mk2_modeuses_PPplusG2, 0 },
 { "PhotoPro2",  	canon_PIXMA_Pro9000mk2_modeuses_PPpro2, 0 },
 { "PhotoProPlat",	canon_PIXMA_Pro9000mk2_modeuses_PPproPlat, 0 },
 { "PhotopaperMatte",	canon_PIXMA_Pro9000mk2_modeuses_PPmatte, 0 },
 { "Coated",		canon_PIXMA_Pro9000mk2_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_Pro9000mk2_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_Pro9000mk2_modeuses_Hagaki, 0 },
 { "DiscCompat",	canon_PIXMA_Pro9000mk2_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_Pro9000mk2_modeuses_disc, 0 },
 { "Boardpaper",	canon_PIXMA_Pro9000mk2_modeuses_board, 0 },
 { "Canvas",	        canon_PIXMA_Pro9000mk2_modeuses_board, 0 },
 { "FineArtPhotoRag",   canon_PIXMA_Pro9000mk2_modeuses_photorag, 0 },
 { "FineArtOther",      canon_PIXMA_Pro9000mk2_modeuses_board, 0 },
 { "FineArtPremiumMatte",canon_PIXMA_Pro9000mk2_modeuses_board, 0 },
 { "FineArtMuseumEtching",canon_PIXMA_Pro9000mk2_modeuses_photorag, 0 },
 { "TShirt",		canon_PIXMA_Pro9000mk2_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_Pro9000mk2_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_Pro9000mk2_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_Pro9000mk2);

/* ----------------------------------- Canon Pro9500 ----------------------------------- */

static const char* canon_PIXMA_Pro9500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_highmono", /*untested*/
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  "600x600dpi_draftmono2",/*untested*/
  NULL
  };

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_PPplusG2[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_PPgloss[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* highest mode not yet supported (R,G inks) */
/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_PPpro2[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* highest mode not yet supported (R,G inks) */
/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_PPproPlat[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_PPmatte[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_inkjetHagaki[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_Hagaki[] = {
   /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_highmono",
  "600x600dpi_mono", /*untested*/
  "600x600dpi_draftmono",/*untested*/
  "600x600dpi_draftmono2",/*untested*/
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_disc[] = {
  /* Mono */
   "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_board[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
  };

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_photorag[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
  };

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_TShirt[] = {
  "600x600dpi_photomono",/* bw=2 for mono */
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500_modeuses_PPother[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_Pro9500_modeuses[] = {
 { "Plain",             canon_PIXMA_Pro9500_modeuses_plain, 0 },
 { "PhotoPlusGloss2",   canon_PIXMA_Pro9500_modeuses_PPplusG2, 0 },
 { "GlossyPaper",	canon_PIXMA_Pro9500_modeuses_PPgloss, 0 },
 { "PhotoProSemiGloss", canon_PIXMA_Pro9500_modeuses_PPplusG2, 0 },
 { "PhotoPro2",  	canon_PIXMA_Pro9500_modeuses_PPpro2, 0 },
 { "PhotoProPlat",	canon_PIXMA_Pro9500_modeuses_PPproPlat, 0 },
 { "PhotopaperMatte",	canon_PIXMA_Pro9500_modeuses_PPmatte, 0 },
 { "Coated",		canon_PIXMA_Pro9500_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_Pro9500_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_Pro9500_modeuses_Hagaki, 0 },
 { "DiscCompat",	canon_PIXMA_Pro9500_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_Pro9500_modeuses_disc, 0 },
 { "Boardpaper",	canon_PIXMA_Pro9500_modeuses_board, 0 },
 { "Canvas",	        canon_PIXMA_Pro9500_modeuses_board, 0 },
 { "FineArtPhotoRag",   canon_PIXMA_Pro9500_modeuses_photorag, 0 },
 { "FineArtOther",      canon_PIXMA_Pro9500_modeuses_board, 0 },
 { "FineArtPremiumMatte",canon_PIXMA_Pro9500_modeuses_board, 0 },
 { "FineArtMuseumEtching",canon_PIXMA_Pro9500_modeuses_photorag, 0 },
 { "TShirt",		canon_PIXMA_Pro9500_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_Pro9500_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_Pro9500_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_Pro9500);

/* ----------------------------------- Canon Pro9500mk2 ----------------------------------- */

static const char* canon_PIXMA_Pro9500mk2_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",
  /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_highmono", /*untested*/
  "600x600dpi_mono",
  "600x600dpi_draftmono",
  "600x600dpi_draftmono2",/*untested*/
  NULL
  };

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_PPplusG2[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_PPgloss[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* highest mode not yet supported (R,G inks) */
/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_PPproPlat[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_PPmatte[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_inkjetHagaki[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_Hagaki[] = {
   /* Mono */
  "600x600dpi_highmono2",
  "600x600dpi_highmono",
  "600x600dpi_mono", /*untested*/
  "600x600dpi_draftmono",/*untested*/
  "600x600dpi_draftmono2",/*untested*/
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_disc[] = {
  /* Mono */
   "600x600dpi_photomono",
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_board[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
  };

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_photorag[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
  };

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_TShirt[] = {
  "600x600dpi_photomono",/* bw=2 for mono */
  NULL
};

/* modes not yet supported (R,G inks) */
static const char* canon_PIXMA_Pro9500mk2_modeuses_PPother[] = {
  /* Mono */
  "600x600dpi_photomono",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_Pro9500mk2_modeuses[] = {
 { "Plain",             canon_PIXMA_Pro9500mk2_modeuses_plain, 0 },
 { "PhotoPlusGloss2",   canon_PIXMA_Pro9500mk2_modeuses_PPplusG2, 0 },
 { "PhotoProPlat",      canon_PIXMA_Pro9500mk2_modeuses_PPproPlat, 0 },
 { "GlossyPaper",	canon_PIXMA_Pro9500mk2_modeuses_PPgloss, 0 },
 { "PhotoProSemiGloss", canon_PIXMA_Pro9500mk2_modeuses_PPplusG2, 0 },
 { "PhotopaperMatte",	canon_PIXMA_Pro9500mk2_modeuses_PPmatte, 0 },
 { "Coated",		canon_PIXMA_Pro9500mk2_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_Pro9500mk2_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_Pro9500mk2_modeuses_Hagaki, 0 },
 { "DiscCompat",	canon_PIXMA_Pro9500mk2_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_Pro9500mk2_modeuses_disc, 0 },
 { "Boardpaper",	canon_PIXMA_Pro9500mk2_modeuses_board, 0 },
 { "Canvas",	        canon_PIXMA_Pro9500mk2_modeuses_board, 0 },
 { "FineArtPhotoRag",   canon_PIXMA_Pro9500mk2_modeuses_photorag, 0 },
 { "FineArtOther",      canon_PIXMA_Pro9500mk2_modeuses_board, 0 },
 { "FineArtPremiumMatte",canon_PIXMA_Pro9500mk2_modeuses_board, 0 },
 { "FineArtMuseumEtching",canon_PIXMA_Pro9500mk2_modeuses_photorag, 0 },
 { "TShirt",		canon_PIXMA_Pro9500mk2_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_Pro9500mk2_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_Pro9500mk2_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_Pro9500mk2);

/* ----------------------------------- Canon MAXIFY iB4000 ----------------------------------- */

static const char* canon_MAXIFY_iB4000_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_mono",
  NULL
};

static const char* canon_MAXIFY_iB4000_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MAXIFY_iB4000_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const canon_modeuse_t canon_MAXIFY_iB4000_modeuses[] = {
  { "Plain",            canon_MAXIFY_iB4000_modeuses_plain, DUPLEX_SUPPORT },
  { "PhotoPlusGloss2",  canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",     canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },/*unsupported*/
  { "PhotoProLuster",   canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "GlossyPaperStandard",canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "Coated",		canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "HagakiA", 	        canon_MAXIFY_iB4000_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "InkjetPhotoHagakiK",canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_MAXIFY_iB4000_modeuses_Hagaki, 0 },
  { "TShirt",		canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },/*unsupported*/
  { "Envelope",		canon_MAXIFY_iB4000_modeuses_Hagaki, 0 }, 
  { "PhotopaperOther",	canon_MAXIFY_iB4000_modeuses_PPplusG2, 0 },/*unsupported*/
};

DECLARE_MODEUSES(canon_MAXIFY_iB4000);

#endif
