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
   i
   iP
   iX
   MP
   MX
   MG
   Pro
*/

/* ----------------------------------- Canon i50  ----------------------------------- */
static const char* canon_PIXMA_i50_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

static const char* canon_PIXMA_i50_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_i50_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_i50_modeuses_PPmatte[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char* canon_PIXMA_i50_modeuses_PPgloss[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photomed",
  "600x600dpi_photo2",
  NULL
};

static const char* canon_PIXMA_i50_modeuses_Hagaki[] = {
  "600x600dpi_high3",
  "600x600dpi_std3",
  "600x600dpi_draft3",
  NULL
};

static const char* canon_PIXMA_i50_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_PIXMA_i50_modeuses_Transparency[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char* canon_PIXMA_i50_modeuses_PPother[] = {
  "600x600dpi_photo2",
  NULL
  };

static const canon_modeuse_t canon_PIXMA_i50_modeuses[] = {
  { "Plain",            canon_PIXMA_i50_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_i50_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_i50_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble",canon_PIXMA_i50_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_PIXMA_i50_modeuses_PPmatte, 0 },
  { "GlossyPaper",	canon_PIXMA_i50_modeuses_PPgloss, 0 },
  { "Coated",		canon_PIXMA_i50_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_i50_modeuses_PPplus, 0 },
  { "Hagaki", 	        canon_PIXMA_i50_modeuses_Hagaki, 0 },
  { "TShirt",		canon_PIXMA_i50_modeuses_TShirt, 0 },
  { "Transparency",	canon_PIXMA_i50_modeuses_Transparency, 0 },
  { "Envelope",		canon_PIXMA_i50_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_i50_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_i50);

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
  { "Plain",            canon_PIXMA_iP3000_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP3000_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP3000_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP3000_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP3000_modeuses_inkjetHagaki, 0 },/*untested*/
  { "Hagaki", 	        canon_PIXMA_iP3000_modeuses_Hagaki, 0 },/*untested*/
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
  { "Plain",            canon_PIXMA_iP3100_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP3100_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP3100_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP3100_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP3100_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP3100_modeuses_Hagaki, 0 },
  { "DiscCompat",	canon_PIXMA_iP3100_modeuses_disc, 0 },
  { "DiscOthers",	canon_PIXMA_iP3100_modeuses_disc, 0 },
  { "TShirt",		canon_PIXMA_iP3100_modeuses_TShirt, 0 },
  { "Envelope",		canon_PIXMA_iP3100_modeuses_Hagaki, 0 },
  { "PhotopaperOther",	canon_PIXMA_iP3100_modeuses_PPother, 0 },/*untested*/
  { "Transparency",     canon_PIXMA_iP3100_modeuses_transparency, 0 },
};

DECLARE_MODEUSES(canon_PIXMA_iP3100);

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
  { "Plain",            canon_PIXMA_iP4000_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP4000_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP4000_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4000_modeuses_inkjetHagaki, 0 },/*untested*/
  { "Hagaki", 	        canon_PIXMA_iP4000_modeuses_Hagaki, 0 },/*untested*/
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
  { "Plain",            canon_PIXMA_iP4100_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP4100_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP4100_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP4100_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4100_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4100_modeuses_Hagaki, 0 },
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
  { "Plain",             canon_PIXMA_iP4200_modeuses_plain, 0 },
  { "GlossyPro",	        canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperPlus",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* mostly not yet supported */
  { "PhotopaperPlusDouble", canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperMatte",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "GlossyPaper",	canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "Coated",		canon_PIXMA_iP4200_modeuses_PPplus, 0 },/* not yet supported */
  { "InkJetHagaki", 	canon_PIXMA_iP4200_modeuses_inkjetHagaki, 0 },/* partially not yet supported */
  { "Hagaki", 	        canon_PIXMA_iP4200_modeuses_Hagaki, 0 },
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
  { "Plain",             canon_PIXMA_iP4300_modeuses_plain, 0 },
  { "GlossyPro",	        canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperPlus",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* mostly not yet supported */
  { "PhotopaperPlusDouble", canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "PhotopaperMatte",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "GlossyPaper",	canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "Coated",		canon_PIXMA_iP4300_modeuses_PPplus, 0 },/* not yet supported */
  { "InkJetHagaki", 	canon_PIXMA_iP4300_modeuses_inkjetHagaki, 0 },/* partially not yet supported */
  { "Hagaki", 	        canon_PIXMA_iP4300_modeuses_Hagaki, 0 },
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
  { "Plain",            canon_PIXMA_iP4500_modeuses_plain, 0 },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4500_modeuses_PPplus, 0 },
  { "GlossyPro",	        canon_PIXMA_iP4500_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP4500_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP4500_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4500_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4500_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4500_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4500_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4500_modeuses_Hagaki, 0 },
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
  { "Plain",            canon_PIXMA_iP4600_modeuses_plain, DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_PIXMA_iP4600_modeuses_PPpro, 0 },
  { "PhotoProPlat",	canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4600_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4600_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4600_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4600_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4600_modeuses_Hagaki, 0 },
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
  { "Plain",            canon_PIXMA_iP4700_modeuses_plain, DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_PIXMA_iP4700_modeuses_PPpro, 0 },
  { "PhotoProPlat",	canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4700_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4700_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4700_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_iP4700_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4700_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_iP4700_modeuses_PPpro, 0 },
  { "OtherPhotoHagakiO",canon_PIXMA_iP4700_modeuses_PPpro, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4700_modeuses_Hagaki, 0 },
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
  { "Plain",            canon_PIXMA_iP4900_modeuses_plain, DUPLEX_MODEREPL },
  { "PhotoPlusGloss2",	canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "PhotoProPlat",	canon_PIXMA_iP4900_modeuses_PPproPlat, 0 },
  { "PhotoProSemiGloss",canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "GlossyPaper",	canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP4900_modeuses_PPmatte, 0 },
  { "Coated",	        canon_PIXMA_iP4900_modeuses_PPmatte, 0 },
  { "HagakiA", 	        canon_PIXMA_iP4900_modeuses_Hagaki, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP4900_modeuses_inkjetHagaki, 0 },
  { "CanonPhotoHagakiK",canon_PIXMA_iP4900_modeuses_PPplusG2, 0 },
  { "Hagaki", 	        canon_PIXMA_iP4900_modeuses_Hagaki, 0 },
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
  { "Plain",            canon_PIXMA_iP5000_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP5000_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP5000_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "Coated",		canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP5000_modeuses_PPplusDS, 0 },/*untested*/
  { "Hagaki", 	        canon_PIXMA_iP5000_modeuses_Hagaki, 0 },/*untested*/
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
  { "Plain",            canon_PIXMA_iP5300_modeuses_plain, 0 },
  { "GlossyPro",	canon_PIXMA_iP5300_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_PIXMA_iP5300_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_PIXMA_iP5300_modeuses_PPplusDS, 0 },
  { "GlossyPaper",	canon_PIXMA_iP5300_modeuses_PPplusDS, 0 },
  { "PhotopaperMatte",	canon_PIXMA_iP5300_modeuses_PPmatte, 0 },
  { "Coated",		canon_PIXMA_iP5300_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_PIXMA_iP5300_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_PIXMA_iP5300_modeuses_Hagaki, 0 },
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
 { "Plain",             canon_PIXMA_iP6000_modeuses_plain, DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP6000_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP6000_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP6000_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP6000_modeuses_inkjetHagaki, 0 },/*untested*/
 { "Hagaki", 	        canon_PIXMA_iP6000_modeuses_Hagaki, 0 },
 { "CD",   	        canon_PIXMA_iP6000_modeuses_disc, 0 },/*NOTE:temporary*/
 { "DiscCompat",	canon_PIXMA_iP6000_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP6000_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP6000_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP6000_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP6000_modeuses_PPother, 0 },
 { "Transparency",      canon_PIXMA_iP6000_modeuses_transparency, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP6000);

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

/* Several modes not supported yet */
static const char* canon_PIXMA_iP6210_modeuses_PPpro[] = {
  "600x600dpi_photo",
  /* Color */
  "600x600dpi_photohigh4",
  NULL
};

/* high mode not supported yet */
static const char* canon_PIXMA_iP6210_modeuses_PPplus[] = {
  "600x600dpi_photo",
  /* Color */
  "600x600dpi_photo3",
  NULL
};

static const char* canon_PIXMA_iP6210_modeuses_PPplusDS[] = {
/* high mode not supported yet */
  "600x600dpi_photo",
  /* Color */
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
  "600x600dpi_photo",
  /* Color */
  "600x600dpi_photo2",/* stand-in */
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
 { "Plain",             canon_PIXMA_iP6700_modeuses_plain, 0 },
 { "GlossyPro",	        canon_PIXMA_iP6700_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP6700_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP6700_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP6700_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP6700_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP6700_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP6700_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iP6700_modeuses_Hagaki, 0 },
 { "FineArtPhotoRag",   canon_PIXMA_iP6700_modeuses_FA, 0 },
 { "FineArtOther",      canon_PIXMA_iP6700_modeuses_FA, 0 },
 { "DiscCompat",	canon_PIXMA_iP6700_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP6700_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP6700_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP6700_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP6700_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP6700);

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
 { "Plain",             canon_PIXMA_iP7500_modeuses_plain, DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP7500_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP7500_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP7500_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP7500_modeuses_PPplus, 0 },
 { "Hagaki", 	        canon_PIXMA_iP7500_modeuses_Hagaki, 0 },
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

/* ----------------------------------- Canon iP8500  ----------------------------------- */
static const char* canon_PIXMA_iP8500_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
  };

/* modes not yet supported */
static const char* canon_PIXMA_iP8500_modeuses_PPpro[] = {
  "600x600dpi_photodraft",/*temporary stand-in: untested*/
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
 { "Plain",             canon_PIXMA_iP8500_modeuses_plain, DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_PIXMA_iP8500_modeuses_PPpro, 0 },/*unsupported*/
 { "PhotopaperPlus",	canon_PIXMA_iP8500_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP8500_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP8500_modeuses_PPpro, 0 },/*unsupported*/
 { "GlossyPaper",	canon_PIXMA_iP8500_modeuses_PPplusDS, 0 },
 { "Coated",		canon_PIXMA_iP8500_modeuses_PPhires, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP8500_modeuses_inkjetHagaki, 0 },/*untested*/
 { "Hagaki", 	        canon_PIXMA_iP8500_modeuses_Hagaki, 0 },/*untested*/
 { "DiscCompat",	canon_PIXMA_iP8500_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP8500_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP8500_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP8500_modeuses_Hagaki, 0 },
 { "Transparency",	canon_PIXMA_iP8500_modeuses_Transparency, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP8500_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP8500);

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
  "600x600dpi_photohigh",
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
 { "Transparency",	canon_PIXMA_iX4000_modeuses_inkjetHagaki, 0 },/*Note: US driver does not have this media */
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

/* photohigh mode disabled in canon-modes.h until cmy (Photo CMY inks only) printing is fixed */
static const char* canon_MULTIPASS_MP150_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh3",
  /*  "600x600dpi_photohigh",*/
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_PPplus[] = {
  /*  "600x600dpi_photohigh",*/
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP150_modeuses_PPplusDS[] = {
  /*  "600x600dpi_photohigh",*/
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
  /*  "600x600dpi_photohigh",*/
  "600x600dpi_photo",/*NOTE: this mode does not exist for PPother, temporary replacement */
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP150_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP150_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "GlossyPro",	        canon_MULTIPASS_MP150_modeuses_PPpro, INKSET_COLOR_SUPPORT },
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

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP190_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  /*"600x600dpi_photohigh",*/
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP190_modeuses_PPplusG2[] = {
  /*"600x600dpi_photohigh",*/
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested*/
  NULL
};

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP190_modeuses_PPmatte[] = {
  /*"600x600dpi_photohigh",*/
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

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP190_modeuses_PPother[] = {
  /*"600x600dpi_photohigh",*/
  "600x600dpi_photo",/* stand-in */
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

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP210_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh2",
  /*"600x600dpi_photohigh",*/
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested here*/
  NULL
};

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP210_modeuses_PPplus[] = {
  /*"600x600dpi_photohigh",*/
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP210_modeuses_PPplusDS[] = {
  /*"600x600dpi_photohigh",*/
  "600x600dpi_photo",
  "600x600dpi_photodraft",/*untested here*/
  NULL
};

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP210_modeuses_PPmatte[] = {
  /*"600x600dpi_photohigh",*/
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

/* one high mode not yet supported */
static const char* canon_MULTIPASS_MP210_modeuses_PPother[] = {
  /*"600x600dpi_photohigh",*/
  "600x600dpi_photo",/* stand-in */
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
 { "Plain",             canon_MULTIPASS_MP240_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_MODEREPL},
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
 { "Plain",             canon_MULTIPASS_MP250_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL | DUPLEX_MODEREPL},
 { "PhotoPlusGloss2",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoPro2",	        canon_MULTIPASS_MP250_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "PhotoProPlat",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotoProSemiGloss", canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "HagakiA", 	        canon_MULTIPASS_MP250_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "InkJetHagaki", 	canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "CanonPhotoHagakiK", canon_MULTIPASS_MP250_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "ProPhotoHagakiP",   canon_MULTIPASS_MP250_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MP250_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP250_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP250_modeuses_Hagaki,INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP250_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP250);

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
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP550_modeuses[] = {
  { "Plain",		canon_MULTIPASS_MP550_modeuses_plain, 0 },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
  { "PhotoPro2",	canon_MULTIPASS_MP550_modeuses_PPplusG2, 0 },
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
 { "Plain",             canon_MULTIPASS_MP610_modeuses_plain, 0 },
 { "GlossyPro",	        canon_MULTIPASS_MP610_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_MULTIPASS_MP610_modeuses_PPplus, 0 },
 { "PhotoPlusGloss2",   canon_MULTIPASS_MP610_modeuses_PPplusG2, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP610_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_MULTIPASS_MP610_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_MULTIPASS_MP610_modeuses_PPplusDS, 0 },
 { "Coated",		canon_MULTIPASS_MP610_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_MULTIPASS_MP610_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP610_modeuses_Hagaki, 0 },
 { "DiscCompat",	canon_MULTIPASS_MP610_modeuses_disc, 0 },
 { "DiscOthers",	canon_MULTIPASS_MP610_modeuses_disc, 0 },
 { "TShirt",		canon_MULTIPASS_MP610_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP610_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP610_modeuses_PPother, 0 },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP610);

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
 { "Plain",             canon_MULTIPASS_MP830_modeuses_plain, 0 },
 { "GlossyPro",	        canon_MULTIPASS_MP830_modeuses_PPpro, 0 },/*not supported yet*/
 { "PhotopaperPlus",	canon_MULTIPASS_MP830_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "GlossyPaper",	canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "PhotopaperMatte",	canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "Coated",		canon_MULTIPASS_MP830_modeuses_PPplusDS, 0 },/*not supported yet*/
 { "InkJetHagaki", 	canon_MULTIPASS_MP830_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP830_modeuses_Hagaki, 0 },
 { "CD",   	        canon_MULTIPASS_MP830_modeuses_plain, 0 },/*NOTE:temporary replacement*/
 { "DiscCompat",	canon_MULTIPASS_MP830_modeuses_disc, 0 },/*not supported yet*/
 { "DiscOthers",	canon_MULTIPASS_MP830_modeuses_disc, 0 },/*not supported yet*/
 { "TShirt",		canon_MULTIPASS_MP830_modeuses_TShirt, 0 },
 { "Envelope",		canon_MULTIPASS_MP830_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_MULTIPASS_MP830_modeuses_PPother, 0 },/*not supported yet*/
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP830);

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
 { "Plain",             canon_MULTIPASS_MP960_modeuses_plain, DUPLEX_MODEREPL },
 { "GlossyPro",	        canon_MULTIPASS_MP960_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_MULTIPASS_MP960_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP960_modeuses_PPplusDS, 0 },
 { "GlossyPaper",	canon_MULTIPASS_MP960_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_MULTIPASS_MP960_modeuses_PPmatte, 0 },
 { "Coated",		canon_MULTIPASS_MP960_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_MULTIPASS_MP960_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_MULTIPASS_MP960_modeuses_Hagaki, 0 },
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
  { "Plain",            canon_MULTIPASS_MP970_modeuses_plain, DUPLEX_MODEREPL },
  { "GlossyPro",	canon_MULTIPASS_MP970_modeuses_PPpro, 0 },
  { "PhotopaperPlus",	canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "PhotopaperPlusDouble", canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "PhotoPlusGloss2",  canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "GlossyPaper",	canon_MULTIPASS_MP970_modeuses_PPplus, 0 },
  { "PhotopaperMatte",	canon_MULTIPASS_MP970_modeuses_PPmatte, 0 },
  { "Coated",		canon_MULTIPASS_MP970_modeuses_PPmatte, 0 },
  { "InkJetHagaki", 	canon_MULTIPASS_MP970_modeuses_inkjetHagaki, 0 },
  { "Hagaki", 	        canon_MULTIPASS_MP970_modeuses_Hagaki, 0 },
  { "DiscCompat",	canon_MULTIPASS_MP970_modeuses_disc, 0 },
  { "DiscOthers",	canon_MULTIPASS_MP970_modeuses_disc, 0 },
  { "TShirt",		canon_MULTIPASS_MP970_modeuses_TShirt, 0 },
  { "Envelope",		canon_MULTIPASS_MP970_modeuses_Hagaki, 0 },
  { "FineArtPhotoRag",	canon_MULTIPASS_MP970_modeuses_FA, 0 },
  { "FineArtOther",	canon_MULTIPASS_MP970_modeuses_FA, 0 },
  { "PhotopaperOther",	canon_MULTIPASS_MP970_modeuses_PPother, 0 },
};

DECLARE_MODEUSES(canon_MULTIPASS_MP970);

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

/* ----------------------------------- Canon Pro9000 ----------------------------------- */

static const char* canon_PIXMA_Pro9000_modeuses_plain[] = {
  "600x600dpi_hig2",
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_std2",
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
  "600x600dpi_high3",
  "600x600dpi_std3",/*Mono High*/
  "600x600dpi_std4",/* bw=2 for mono */
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
  "600x600dpi_tshirt",/* bw=2 for mono */
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
  "600x600dpi_high2",/* bw=2 for mono */
  "600x600dpi_std2",/* bw=2 for mono */
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
  "600x600dpi_tshirt",/* bw=2 for mono */
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

#endif
