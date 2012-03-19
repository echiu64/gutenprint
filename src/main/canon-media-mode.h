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
  const char* name; /* unstranslated media name */
  unsigned char* mode_name_list; /* untranslated mode names */
  unsigned int use_flags;
} canon_modeuse_t;

typedef struct {
  const char* name;
  short count;
  const canon_modeuse_t *modeuses;
} canon_modeuselist_t;

#define DECLARE_MODEUSES(name)                              \
static const canon_modeuselist_t name##_modeuselist = {     \
  #name,                                                    \
  sizeof(name##_modeuses) / sizeof(canon_modeuse_t),        \
  name##_modeuses                                           \
}


/* Canon MP610 */
static const char *canon_MULTIPASS_MP610_modeuses_plain[] = {
  "600x600dpi_high",
  "600x600dpi",
  "300x300dpi",
  "300x300dpi_draft",
  NULL
};

/* missing highest resolution mode as not yet implemented */
static const char *canon_MULTIPASS_MP610_modeuses_PPpro[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_PPplus[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photodraft",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_PPplusG2[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  "600x600dpi_photodraft",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_PPplusDS[] = {
  "600x600dpi_photohigh",
  "600x600dpi_photo",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_PPmatte[] = {
  "600x600dpi_photohigh2",
  "600x600dpi_photo2",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_inkjetHagaki[] = {
  "600x600dpi_photohigh3",
  "600x600dpi_photo3",
  "600x600dpi_photodraft3",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_Hagaki[] = {
  "600x600dpi_high2",
  "600x600dpi_std2",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_disc[] = {
  "600x600dpi_photohigh4",
  "600x600dpi_photo4",
  "600x600dpi_photodraft4",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_TShirt[] = {
  "600x600dpi_tshirt",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_PPother[] = {
  "600x600dpi_photo",
  NULL
};

static const canon_modeuse_t canon_MULTIPASS_MP610_modeuses[] = {
 { "Plain",             &canon_MULTIPASS_MP610_modeuses_plain, 0 },
 { "PhotopaperPro",	&canon_MULTIPASS_MP610_modeuses_PPpro, 0 },
 { "PhotopaperPlus",	&canon_MULTIPASS_MP610_modeuses_PPplus, 0 },
 { "PhotoPlusGloss2",   &canon_MULTIPASS_MP610_modeuses_PPplusG2, 0 },
 { "PhotopaperPlusDouble", &canon_MULTIPASS_MP610_modeuses_PPplusDS, 0 },
 { "PhotopaperMatte",	&canon_MULTIPASS_MP610_modeuses_PPmatte, 0 },
 { "GlossyPaper",	&canon_MULTIPASS_MP610_modeuses_PPplusDS, 0 },
 { "Coated",		&canon_MULTIPASS_MP610_modeuses_PPmatte, 0 },
 { "InkJetHagaki", 	&canon_MULTIPASS_MP610_modeuses_inkjetHagaki, 0 },
 { "Hagaki", 	        &canon_MULTIPASS_MP610_modeuses_Hagaki, 0 },
 { "DiscCompat",	&canon_MULTIPASS_MP610_modeuses_disc, 0 },
 { "DiscOthers",	&canon_MULTIPASS_MP610_modeuses_disc, 0 },
 { "TShirt",		&canon_MULTIPASS_MP610_modeuses_TShirt, 0 },
 { "Envelope",		&canon_MULTIPASS_MP610_modeuses_Hagaki, 0 },
 { "PhotopaperOther",	&canon_MULTIPASS_MP610_modeuses_PPother, 0 },
};
DECLARE_MODEUSES(canon_MULTIPASS_MP610);

/*
static const char *canon_MULTIPASS_MP610_modeuses_plain_media[] = {
  "Plain",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_photohigh_media[] = {
  "PhotopaperPro",	 
  "PhotopaperPlus",	 
  "PhotoPlusGloss2",	 
  "PhotopaperPlusDouble", 
  "GlossyPaper",		 
  "InkJetHagaki",	 
  "PhotopaperOther",      
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_photo_media[] = {
  "PhotopaperPro",	 
  "PhotopaperPlus",	 
  "PhotoPlusGloss2",	 
  "PhotopaperPlusDouble", 
  "PhotopaperMatte",
  "GlossyPaper",		 
  "Coated",
  "InkJetHagaki",	 
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_photodraft_media[] = {
  "PhotopaperPlus",	 
  "PhotoPlusGloss2",	 
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_photohigh2_media[] = {
  "PhotopaperMatte",
  "Coated",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_photohigh3_media[] = {
  "Hagaki",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_photohigh4_media[] = {
  "DiscCompat",
  "DiscOthers",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_plain2_media[] = {
  "Hagaki",
  "Envelope",
  NULL
};

static const char *canon_MULTIPASS_MP610_modeuses_600x600dpi_tshirt_media[] = {
  "TShirt",
  NULL
};

static const canon_modeuselist_t canon_MULTIPASS_MP610_modeuselist[] = {
 { "600x600dpi_high", &canon_MULTIPASS_MP610_modeuses_plain_media, 0 },
 { "600x600dpi", &canon_MULTIPASS_MP610_modeuses_plain_media, 0 },
 { "300x300dpi", &canon_MULTIPASS_MP610_modeuses_plain_media, 0 },
 { "300x300dpi_draft", &canon_MULTIPASS_MP610_modeuses_plain_media, 0 },
 { "600x600dpi_photohigh", &canon_MULTIPASS_MP610_modeuses_600x600photohigh_media, 0 },
 { "600x600dpi_photo", &canon_MULTIPASS_MP610_modeuses_600x600photo_media, 0 },
 { "600x600dpi_photodraft", &canon_MULTIPASS_MP610_modeuses_600x600photodraft_media, 0 },
 { "600x600dpi_photohigh2", &canon_MULTIPASS_MP610_modeuses_600x600photohigh2_media, 0 },
 { "600x600dpi_photo2", &canon_MULTIPASS_MP610_modeuses_600x600photohigh2_media, 0 },
 { "600x600dpi_photohigh3", &canon_MULTIPASS_MP610_modeuses_600x600photohigh3_media, 0 },
 { "600x600dpi_photo3", &canon_MULTIPASS_MP610_modeuses_600x600photohigh3_media, 0 },
 { "600x600dpi_photodraft3", &canon_MULTIPASS_MP610_modeuses_600x600photohigh3_media, 0 },
 { "600x600dpi_photohigh4", &canon_MULTIPASS_MP610_modeuses_600x600photohigh4_media, 0 },
 { "600x600dpi_photo4", &canon_MULTIPASS_MP610_modeuses_600x600photohigh4_media, 0 },
 { "600x600dpi_photodraft4", &canon_MULTIPASS_MP610_modeuses_600x600photohigh4_media, 0 },
 { "600x600dpi_high2", &canon_MULTIPASS_MP610_modeuses_plain2_media, 0 },
 { "600x600dpi_std2", &canon_MULTIPASS_MP610_modeuses_plain2_media, 0 },
 { "600x600dpi_tshirt", &canon_MULTIPASS_MP610_modeuses_tshirt_media, 0 },
};
*/


#endif
