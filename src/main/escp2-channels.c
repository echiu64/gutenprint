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

static const char standard_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"1.00 1.10 1.20 1.30 1.40 1.50 1.60 1.70 "  /* C */
"1.80 1.90 1.90 1.90 1.70 1.50 1.30 1.10 "  /* B */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
"1.00 1.00 1.00 1.10 1.20 1.30 1.40 1.50 "  /* Y */
"1.50 1.40 1.30 1.20 1.10 1.00 1.00 1.00 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char standard_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"0.50 0.60 0.70 0.80 0.90 0.86 0.82 0.79 "  /* C */
"0.78 0.80 0.83 0.87 0.90 0.95 1.05 1.15 "  /* B */
"1.30 1.25 1.20 1.15 1.12 1.09 1.06 1.03 "  /* M */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
"1.00 0.90 0.80 0.70 0.65 0.60 0.55 0.52 "  /* Y */
"0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char standard_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
"0.00 0.05 0.04 0.01 -.03 -.10 -.18 -.26 "  /* C */
"-.35 -.43 -.40 -.32 -.25 -.18 -.10 -.07 "  /* B */
"0.00 -.04 -.09 -.13 -.18 -.23 -.27 -.31 "  /* M */
"-.35 -.38 -.30 -.23 -.15 -.08 0.00 -.02 "  /* R */
"0.00 0.08 0.10 0.08 0.05 0.03 -.03 -.12 "  /* Y */
"-.20 0.17 -.20 -.17 -.15 -.12 -.10 -.08 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


#define DECLARE_INK_CHANNEL(name)				\
static const ink_channel_t name##_channels =			\
{								\
  name##_subchannels,						\
  sizeof(name##_subchannels) / sizeof(physical_subchannel_t),	\
}

static const physical_subchannel_t standard_black_subchannels[] =
{
  { 0, -1, 0 }
};

DECLARE_INK_CHANNEL(standard_black);

static const physical_subchannel_t x80_black_subchannels[] =
{
  { 0, -1, 48 }
};

DECLARE_INK_CHANNEL(x80_black);

static const physical_subchannel_t c80_black_subchannels[] =
{
  { 0, -1, 0 }
};

DECLARE_INK_CHANNEL(c80_black);

static const physical_subchannel_t standard_cyan_subchannels[] =
{
  { 2, -1, 0 }
};

DECLARE_INK_CHANNEL(standard_cyan);

static const physical_subchannel_t x80_cyan_subchannels[] =
{
  { 2, -1, 96 }
};

DECLARE_INK_CHANNEL(x80_cyan);

static const physical_subchannel_t c80_cyan_subchannels[] =
{
  { 2, -1, 0 }
};

DECLARE_INK_CHANNEL(c80_cyan);

static const physical_subchannel_t standard_magenta_subchannels[] =
{
  { 1, -1, 0 }
};

DECLARE_INK_CHANNEL(standard_magenta);

static const physical_subchannel_t x80_magenta_subchannels[] =
{
  { 1, -1, 48 }
};

DECLARE_INK_CHANNEL(x80_magenta);

static const physical_subchannel_t c80_magenta_subchannels[] =
{
  { 1, -1, 120 }
};

DECLARE_INK_CHANNEL(c80_magenta);

static const physical_subchannel_t standard_yellow_subchannels[] =
{
  { 4, -1, 0 }
};

DECLARE_INK_CHANNEL(standard_yellow);

static const physical_subchannel_t x80_yellow_subchannels[] =
{
  { 4, -1, 0 }
};

DECLARE_INK_CHANNEL(x80_yellow);

static const physical_subchannel_t c80_yellow_subchannels[] =
{
  { 4, -1, 240 }
};

DECLARE_INK_CHANNEL(c80_yellow);

static const physical_subchannel_t photo_black_subchannels[] =
{
  { 0, 0, 0 }
};

DECLARE_INK_CHANNEL(photo_black);

static const physical_subchannel_t extended_black_subchannels[] =
{
  { 0, 1, 0 }
};

DECLARE_INK_CHANNEL(extended_black);

static const physical_subchannel_t photo_cyan_subchannels[] =
{
  { 2, 0, 0 },
  { 2, 1, 0 }
};

DECLARE_INK_CHANNEL(photo_cyan);

static const physical_subchannel_t extended_cyan_subchannels[] =
{
  { 2, 1, 0 }
};

DECLARE_INK_CHANNEL(extended_cyan);

static const physical_subchannel_t photo_magenta_subchannels[] =
{
  { 1, 0, 0 },
  { 1, 1, 0 }
};

DECLARE_INK_CHANNEL(photo_magenta);

static const physical_subchannel_t extended_magenta_subchannels[] =
{
  { 1, 1, 0 }
};

DECLARE_INK_CHANNEL(extended_magenta);

static const physical_subchannel_t photo_yellow_subchannels[] =
{
  { 4, 0, 0 }
};

DECLARE_INK_CHANNEL(photo_yellow);

static const physical_subchannel_t extended_yellow_subchannels[] =
{
  { 4, 1, 0 }
};

DECLARE_INK_CHANNEL(extended_yellow);

/* For Japanese 7-color printers, with dark yellow */
static const physical_subchannel_t photo2_yellow_subchannels[] =
{
  { 4, 0, 0 },
  { 4, 2, 0 }
};

DECLARE_INK_CHANNEL(photo2_yellow);

static const physical_subchannel_t photo2_black_subchannels[] =
{
  { 0, 0, 0 },
  { 0, 1, 0 }
};

DECLARE_INK_CHANNEL(photo2_black);

static const physical_subchannel_t quadtone_subchannels[] =
{
  { 4, -1, 0 },
  { 1, -1, 0 },
  { 2, -1, 0 },
  { 0, -1, 0 }
};

DECLARE_INK_CHANNEL(quadtone);

static const physical_subchannel_t c80_quadtone_subchannels[] =
{
  { 4, -1, 240 },
  { 1, -1, 120 },
  { 2, -1, 0 },
  { 0, -1, 0 }
};

DECLARE_INK_CHANNEL(c80_quadtone);

static const escp2_inkname_t three_color_composite_inkset =
{
  "RGB", N_("Three Color Composite"), 1, INKSET_CMYK, 0, 0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &standard_cyan_channels,
    &standard_magenta_channels, &standard_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t x80_three_color_composite_inkset =
{
  "RGB", N_("Three Color Composite"), 1, INKSET_CMYK, 0, 0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &x80_cyan_channels,
    &x80_magenta_channels, &x80_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t c80_three_color_composite_inkset =
{
  "RGB", N_("Three Color Composite"), 1, INKSET_CMYK, 0, 0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &c80_cyan_channels,
    &c80_magenta_channels, &c80_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t four_color_standard_inkset =
{
  "CMYK", N_("Four Color Standard"), 1, INKSET_CMYK, .25, 1.0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &standard_black_channels, &standard_cyan_channels,
    &standard_magenta_channels, &standard_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t x80_four_color_standard_inkset =
{
  "CMYK", N_("Four Color Standard"), 1, INKSET_CMYK, .25, 1.0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &x80_black_channels, &x80_cyan_channels,
    &x80_magenta_channels, &x80_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t c80_four_color_standard_inkset =
{
  "CMYK", N_("Four Color Standard"), 1, INKSET_CMYK, .25, 1.0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &c80_black_channels, &c80_cyan_channels,
    &c80_magenta_channels, &c80_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t six_color_photo_inkset =
{
  "PhotoCMYK", N_("Six Color Photo"), 1, INKSET_CcMmYK, .5, 1.0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  },
  {
    NULL, "LightCyanTransition", "LightMagentaTransition", NULL
  }
};

static const escp2_inkname_t five_color_photo_composite_inkset =
{
  "PhotoCMY", N_("Five Color Photo Composite"), 1, INKSET_CcMmYK, 0, 0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  },
  {
    NULL, "LightCyanTransition", "LightMagentaTransition", NULL
  }
};

static const escp2_inkname_t j_seven_color_enhanced_inkset =
{
  "Photo7J", N_("Seven Color Enhanced"), 1, INKSET_CcMmYyK, .5, 1.0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo2_yellow_channels
  },
  {
    NULL,
    "LightCyanTransition", "LightMagentaTransition", "DarkYellowTransition"
  }
};

static const escp2_inkname_t j_six_color_enhanced_composite_inkset =
{
  "PhotoEnhanceJ", N_("Six Color Enhanced Composite"), 1, INKSET_CcMmYyK, .5, 1.0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &standard_cyan_channels,
    &standard_magenta_channels, &standard_yellow_channels
  },
  {
    NULL,
    "LightCyanTransition", "LightMagentaTransition", "DarkYellowTransition"
  }
};

static const escp2_inkname_t seven_color_photo_inkset =
{
  "PhotoCMYK7", N_("Seven Color Photo"), 1, INKSET_CcMmYKk, .05 , 1.0, 4,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo2_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  },
  {
    "GrayTransition", "LightCyanTransition", "LightMagentaTransition", NULL
  }
};

static const escp2_inkname_t two_color_grayscale_inkset =
{
  "Gray2", N_("Two Level Grayscale"), 0, INKSET_CcMmYKk, 0, 0, 1,
  NULL, NULL, NULL,
  {
    &photo2_black_channels, NULL, NULL, NULL
  },
  {
    "GrayTransition", NULL, NULL, NULL
  }
};

static const escp2_inkname_t one_color_extended_inkset =
{
  "PhysicalBlack", N_("One Color Raw"), 0, INKSET_EXTENDED, 0, 0, 1,
  NULL, NULL, NULL,
  {
    &standard_black_channels
  },
  {
    NULL
  }
};

static const escp2_inkname_t two_color_extended_inkset =
{
  "PhysicalBlack2", N_("Two Color Raw"), 1, INKSET_EXTENDED, 0, 0, 2,
  NULL, NULL, NULL,
  {
    &photo_black_channels, &extended_black_channels,
  },
  {
    NULL, NULL
  }
};

static const escp2_inkname_t three_color_extended_inkset =
{
  "PhysicalCMY", N_("Three Color Raw"), 1, INKSET_EXTENDED, 0, 0, 3,
  NULL, NULL, NULL,
  {
    &standard_cyan_channels, &standard_magenta_channels,
    &standard_yellow_channels
  },
  {
    NULL, NULL, NULL
  }
};

static const escp2_inkname_t x80_three_color_extended_inkset =
{
  "PhysicalCMY", N_("Three Color Raw"), 1, INKSET_EXTENDED, 0, 0, 3,
  NULL, NULL, NULL,
  {
    &x80_cyan_channels, &x80_magenta_channels, &x80_yellow_channels
  },
  {
    NULL, NULL, NULL
  }
};

static const escp2_inkname_t c80_three_color_extended_inkset =
{
  "PhysicalCMY", N_("Three Color Raw"), 1, INKSET_EXTENDED, 0, 0, 3,
  NULL, NULL, NULL,
  {
    &c80_cyan_channels, &c80_magenta_channels, &c80_yellow_channels
  },
  {
    NULL, NULL, NULL
  }
};

static const escp2_inkname_t four_color_extended_inkset =
{
  "PhysicalCMYK", N_("Four Color Raw"), 1, INKSET_EXTENDED, 0, 0, 4,
  NULL, NULL, NULL,
  {
    &standard_black_channels, &standard_cyan_channels,
    &standard_magenta_channels, &standard_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t x80_four_color_extended_inkset =
{
  "PhysicalCMYK", N_("Four Color Raw"), 1, INKSET_EXTENDED, 0, 0, 4,
  NULL, NULL, NULL,
  {
    &x80_black_channels, &x80_cyan_channels,
    &x80_magenta_channels, &x80_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t c80_four_color_extended_inkset =
{
  "PhysicalCMYK", N_("Four Color Raw"), 1, INKSET_EXTENDED, 0, 0, 4,
  NULL, NULL, NULL,
  {
    &c80_black_channels, &c80_cyan_channels,
    &c80_magenta_channels, &c80_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t five_color_extended_inkset =
{
  "PhysicalCcMmY", N_("Five Color Raw"), 1, INKSET_EXTENDED, 0, 0, 5,
  NULL, NULL, NULL,
  {
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t six_color_extended_inkset =
{
  "PhysicalCcMmYK", N_("Six Color Raw"), 1, INKSET_EXTENDED, 0, 0, 6,
  NULL, NULL, NULL,
  {
    &photo_black_channels, &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t seven_color_extended_inkset =
{
  "PhysicalCcMmYKk", N_("Seven Color Raw"), 1, INKSET_EXTENDED, 0, 0, 7,
  NULL, NULL, NULL,
  {
    &photo_black_channels, &extended_black_channels, &standard_cyan_channels,
    &extended_cyan_channels, &standard_magenta_channels,
    &extended_magenta_channels, &photo_yellow_channels
  },
  {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL
  }
};

static const escp2_inkname_t piezo_quadtone_inkset =
{
  "Quadtone", N_("Quadtone"), 0, INKSET_PIEZO_QUADTONE, 0, 0, 1,
  NULL, NULL, NULL,
  {
    &quadtone_channels, NULL, NULL, NULL
  },
  {
    "GrayTransition", NULL, NULL, NULL
  }
};

static const escp2_inkname_t c80_piezo_quadtone_inkset =
{
  "Quadtone", N_("Quadtone"), 0, INKSET_PIEZO_QUADTONE, 0, 0, 1,
  NULL, NULL, NULL,
  {
    &c80_quadtone_channels, NULL, NULL, NULL
  },
  {
    "GrayTransition", NULL, NULL, NULL
  }
};

#define DECLARE_INKLIST(name)				\
const inklist_t stpi_escp2_##name##_inklist =		\
{							\
  name##_ink_types,					\
  sizeof(name##_ink_types) / sizeof(escp2_inkname_t *),	\
}							\


static const escp2_inkname_t *const cmy_ink_types[] =
{
  &three_color_composite_inkset
};

DECLARE_INKLIST(cmy);

static const escp2_inkname_t *const standard_ink_types[] =
{
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &piezo_quadtone_inkset,
  &one_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
};

DECLARE_INKLIST(standard);

static const escp2_inkname_t *const c80_ink_types[] =
{
  &c80_four_color_standard_inkset,
  &c80_three_color_composite_inkset,
  &c80_piezo_quadtone_inkset,
  &one_color_extended_inkset,
  &c80_three_color_extended_inkset,
  &c80_four_color_extended_inkset,
};

DECLARE_INKLIST(c80);

static const escp2_inkname_t *const x80_ink_types[] =
{
  &x80_four_color_standard_inkset,
  &x80_three_color_composite_inkset,
  &one_color_extended_inkset,
  &x80_three_color_extended_inkset,
  &x80_four_color_extended_inkset,
};

DECLARE_INKLIST(x80);

static const escp2_inkname_t *const photo_ink_types[] =
{
  &six_color_photo_inkset,
  &five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &piezo_quadtone_inkset,
  &one_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &five_color_extended_inkset,
  &six_color_extended_inkset,
};

DECLARE_INKLIST(photo);

static const escp2_inkname_t *const photo7_japan_ink_types[] =
{
  &j_seven_color_enhanced_inkset,
  &j_six_color_enhanced_composite_inkset,
  &six_color_photo_inkset,
  &five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &piezo_quadtone_inkset,
  &one_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &five_color_extended_inkset,
  &six_color_extended_inkset,
  &seven_color_extended_inkset,
};

DECLARE_INKLIST(photo7_japan);

static const escp2_inkname_t *const photo7_ink_types[] =
{
  &seven_color_photo_inkset,
  &six_color_photo_inkset,
  &five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &two_color_grayscale_inkset,
  &one_color_extended_inkset,
  &two_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &five_color_extended_inkset,
  &six_color_extended_inkset,
  &seven_color_extended_inkset,
};

DECLARE_INKLIST(photo7);
