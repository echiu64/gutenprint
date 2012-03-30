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
static const canon_modeuselist_t name##_modeuselist = {     \
  #name,                                                    \
  sizeof(name##_modeuses) / sizeof(canon_modeuse_t),        \
  name##_modeuses                                           \
}


/* ----------------------------------- Canon iP4000  ----------------------------------- */
static const char* canon_PIXMA_iP4000_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "600x600dpi_draft",/*untested*/
  "300x300dpi_high",/*untested*/
  "300x300dpi",
  "300x300dpi_draft",
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
 { "Plain",             canon_PIXMA_iP4000_modeuses_plain, 0 },
 { "PhotopaperPro",	canon_PIXMA_iP4000_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP4000_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
 { "GlossyPaper",	canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
 { "Coated",		canon_PIXMA_iP4000_modeuses_PPplusDS, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP4000_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        canon_PIXMA_iP4000_modeuses_Hagaki, 0 },
 { "CD",   	        canon_PIXMA_iP4000_modeuses_plain, 0 },/*NOTE:temporary replacement*/
 { "DiscCompat",	canon_PIXMA_iP4000_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP4000_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP4000_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP4000_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP4000_modeuses_PPother, 0 },
 { "Transparency",      canon_PIXMA_iP4000_modeuses_transparency, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP4000);

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
 { "Plain",             canon_PIXMA_iP6000_modeuses_plain, 0 },
 { "PhotopaperPro",	canon_PIXMA_iP6000_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "PhotopaperPlusDouble", canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "PhotopaperMatte",	canon_PIXMA_iP6000_modeuses_PPmatte, 0 },
 { "GlossyPaper",	canon_PIXMA_iP6000_modeuses_PPplus, 0 },
 { "Coated",		canon_PIXMA_iP6000_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	canon_PIXMA_iP6000_modeuses_inkjetHagaki, 0 },/*untested*/
 { "Hagaki", 	        canon_PIXMA_iP6000_modeuses_Hagaki, 0 },
 { "DiscCompat",	canon_PIXMA_iP6000_modeuses_disc, 0 },
 { "DiscOthers",	canon_PIXMA_iP6000_modeuses_disc, 0 },
 { "TShirt",		canon_PIXMA_iP6000_modeuses_TShirt, 0 },
 { "Envelope",		canon_PIXMA_iP6000_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	canon_PIXMA_iP6000_modeuses_PPother, 0 },
 { "Transparency",      canon_PIXMA_iP6000_modeuses_transparency, 0 },
 };

DECLARE_MODEUSES(canon_PIXMA_iP6000);

/* ----------------------------------- Canon MP450  ----------------------------------- */
static const char* canon_MULTIPASS_MP450_modeuses_plain[] = {
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
static const char* canon_MULTIPASS_MP450_modeuses_PPpro[] = {
  "1200x1200dpi_photohigh3",
  /*  "600x600dpi_photohigh",*/
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP450_modeuses_PPplus[] = {
  /*  "600x600dpi_photohigh",*/
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char* canon_MULTIPASS_MP450_modeuses_PPplusDS[] = {
  /*  "600x600dpi_photohigh",*/
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP450_modeuses_PPgloss[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo",
  NULL
};

static const char* canon_MULTIPASS_MP450_modeuses_Hagaki[] = {
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

static const char* canon_MULTIPASS_MP450_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char* canon_MULTIPASS_MP450_modeuses_PPother[] = {
  /*  "600x600dpi_photohigh",*/
  "600x600dpi_photo",/*NOTE: this mode does not exist for PPother, temporary replacement */
  NULL
  };

static const canon_modeuse_t canon_MULTIPASS_MP450_modeuses[] = {
 { "Plain",             canon_MULTIPASS_MP450_modeuses_plain, INKSET_BLACK_SUPPORT | INKSET_COLOR_SUPPORT | INKSET_BLACK_MODEREPL | INKSET_COLOR_MODEREPL },
 { "PhotopaperPro",	canon_MULTIPASS_MP450_modeuses_PPpro, INKSET_COLOR_SUPPORT },
 { "PhotopaperPlus",	canon_MULTIPASS_MP450_modeuses_PPplus, INKSET_COLOR_SUPPORT },
 { "PhotopaperPlusDouble", canon_MULTIPASS_MP450_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
 { "PhotopaperMatte",	canon_MULTIPASS_MP450_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
 { "GlossyPaper",	canon_MULTIPASS_MP450_modeuses_PPgloss, INKSET_COLOR_SUPPORT },
 { "Coated",		canon_MULTIPASS_MP450_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
 { "InkJetHagaki", 	canon_MULTIPASS_MP450_modeuses_PPplusDS, INKSET_COLOR_SUPPORT },
 { "Hagaki", 	        canon_MULTIPASS_MP450_modeuses_Hagaki, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
 { "TShirt",		canon_MULTIPASS_MP450_modeuses_TShirt, INKSET_COLOR_SUPPORT },
 { "Envelope",		canon_MULTIPASS_MP450_modeuses_Hagaki, INKSET_COLOR_SUPPORT | INKSET_COLOR_MODEREPL },
 { "PhotopaperOther",	canon_MULTIPASS_MP450_modeuses_PPother, INKSET_COLOR_SUPPORT },
 };

DECLARE_MODEUSES(canon_MULTIPASS_MP450);

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
 { "PhotopaperPro",	canon_MULTIPASS_MP610_modeuses_PPpro, 0 },
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

#endif
