/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
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
#include <sys/types.h>
#include "../../lib/libprintut.h"
#include <string.h>

#include "print-intl.h"
#include "print_gimp.h"

/* Concrete type to represent image */
typedef struct
{
  const guchar *data;
  gint w, h, bpp;
  gint32 real_bpp;
} thumbnail_image_t;

static const char *Thumbnail_get_appname(stp_image_t *image);
static void Thumbnail_progress_conclude(stp_image_t *image);
static void Thumbnail_note_progress(stp_image_t *image,
				    double current, double total);
static void Thumbnail_progress_init(stp_image_t *image);
static stp_image_status_t Thumbnail_get_row(stp_image_t *image,
					    unsigned char *data, int row);
static int Thumbnail_height(stp_image_t *image);
static int Thumbnail_width(stp_image_t *image);
static int Thumbnail_bpp(stp_image_t *image);
static void Thumbnail_reset(stp_image_t *image);
static void Thumbnail_init(stp_image_t *image);

static stp_image_t theImage =
{
  Thumbnail_init,
  Thumbnail_reset,
  Thumbnail_bpp,
  Thumbnail_width,
  Thumbnail_height,
  Thumbnail_get_row,
  Thumbnail_get_appname,
  Thumbnail_progress_init,
  Thumbnail_note_progress,
  Thumbnail_progress_conclude,
  NULL
};

stp_image_t *
Image_Thumbnail_new(const guchar *data, gint w, gint h, gint bpp)
{
  thumbnail_image_t *im = xmalloc(sizeof(thumbnail_image_t));
  memset(im, 0, sizeof(thumbnail_image_t));
  im->data = data;
  im->w = w;
  im->h = h;
  im->bpp = bpp;

  theImage.rep = im;
  theImage.reset(&theImage);
  return &theImage;
}

static int
Thumbnail_bpp(stp_image_t *image)
{
  thumbnail_image_t *im = (thumbnail_image_t *) (image->rep);
  return im->bpp;
}

static int
Thumbnail_width(stp_image_t *image)
{
  thumbnail_image_t *im = (thumbnail_image_t *) (image->rep);
  return im->w;
}

static int
Thumbnail_height(stp_image_t *image)
{
  thumbnail_image_t *im = (thumbnail_image_t *) (image->rep);
  return im->h;
}

static stp_image_status_t
Thumbnail_get_row(stp_image_t *image, unsigned char *data, int row)
{
  thumbnail_image_t *im = (thumbnail_image_t *) (image->rep);
  const guchar *where = im->data + (row * im->w * im->bpp);
  memcpy(data, where, im->w * im->bpp);
  return STP_IMAGE_OK;
}

static void
Thumbnail_init(stp_image_t *image)
{
  /* Nothing to do. */
}

static void
Thumbnail_reset(stp_image_t *image)
{
}

static void
Thumbnail_progress_init(stp_image_t *image)
{
}

static void
Thumbnail_note_progress(stp_image_t *image, double current, double total)
{
}

static void
Thumbnail_progress_conclude(stp_image_t *image)
{
}

static const char *
Thumbnail_get_appname(stp_image_t *image)
{
  static char pluginname[] = PLUG_IN_NAME " plug-in V" PLUG_IN_VERSION
    " for GIMP";
  return pluginname;
}

/*
 * End of "$Id$".
 */
