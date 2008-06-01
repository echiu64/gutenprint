/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU eral Public License as published by the Free
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
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include "print-escp2.h"

#define DECLARE_INPUT_SLOT(name)				\
static const input_slot_list_t name##_input_slot_list =		\
{								\
  #name,							\
  name##_input_slots,						\
  sizeof(name##_input_slots) / sizeof(const input_slot_t),	\
}

static const input_slot_t standard_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    0,
    0,
    { 16, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Roll",
    N_("Roll Feed"),
    0,
    1,
    0,
    ROLL_FEED_DONT_EJECT,
    { 16, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001" },
    { 6, "IR\002\000\000\002" }
  }
};

DECLARE_INPUT_SLOT(standard_roll_feed);

static const input_slot_t cutter_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    0,
    0,
    { 16, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "RollCutPage",
    N_("Roll Feed (cut each page)"),
    0,
    1,
    0,
    ROLL_FEED_CUT_ALL,
    { 16, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001" },
    { 6, "IR\002\000\000\002" }
  },
  {
    "RollCutNone",
    N_("Roll Feed (do not cut)"),
    0,
    1,
    0,
    ROLL_FEED_DONT_EJECT,
    { 16, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001" },
    { 6, "IR\002\000\000\002" }
  }
};

DECLARE_INPUT_SLOT(cutter_roll_feed);

static const input_slot_t cd_cutter_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\001\377" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Manual",
    N_("Manual Feed"),
    0,
    0,
    0,
    0,
    { 36, "PM\002\000\000\000IR\002\000\000\001EX\006\000\000\000\000\000\005\000FP\003\000\000\000\000PP\003\000\000\002\001" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "CD",
    N_("Print to CD"),
    1,
    0,
    0,
    0,
    { 36, "PM\002\000\000\000IR\002\000\000\001EX\006\000\000\000\000\000\005\000FP\003\000\000\000\000PP\003\000\000\002\001" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "RollCutPage",
    N_("Roll Feed (cut each page)"),
    0,
    1,
    0,
    ROLL_FEED_CUT_ALL,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001PP\003\000\000\001\377" },
    { 6, "IR\002\000\000\002" }
  },
  {
    "RollCutNone",
    N_("Roll Feed (do not cut)"),
    0,
    1,
    0,
    ROLL_FEED_DONT_EJECT,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001PP\003\000\000\001\377" },
    { 6, "IR\002\000\000\002" }
  }
};

DECLARE_INPUT_SLOT(cd_cutter_roll_feed);

static const input_slot_t cd_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\001\377" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Manual",
    N_("Manual Feed"),
    0,
    0,
    0,
    0,
    { 36, "PM\002\000\000\000IR\002\000\000\001EX\006\000\000\000\000\000\005\000FP\003\000\000\000\000PP\003\000\000\002\001" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "CD",
    N_("Print to CD"),
    1,
    0,
    0,
    0,
    { 36, "PM\002\000\000\000IR\002\000\000\001EX\006\000\000\000\000\000\005\000FP\003\000\000\000\000PP\003\000\000\002\001" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Roll",
    N_("Roll Feed"),
    0,
    1,
    0,
    ROLL_FEED_DONT_EJECT,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001PP\003\000\000\001\377" },
    { 6, "IR\002\000\000\002" }
  }
};

DECLARE_INPUT_SLOT(cd_roll_feed);

static const input_slot_t r2400_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\001\377" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Velvet",
    N_("Manual Sheet Guide"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\003\000" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Matte",
    N_("Manual Feed (Front)"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\002\000" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Roll",
    N_("Roll Feed"),
    0,
    1,
    0,
    ROLL_FEED_DONT_EJECT,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001PP\003\000\000\003\001" },
    { 6, "IR\002\000\000\002" }
  }
};

DECLARE_INPUT_SLOT(r2400);

static const input_slot_t r1800_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\001\377" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Velvet",
    N_("Manual Sheet Guide"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\003\000" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Matte",
    N_("Manual Feed (Front)"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\002\000" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Roll",
    N_("Roll Feed"),
    0,
    1,
    0,
    ROLL_FEED_DONT_EJECT,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\001PP\003\000\000\003\001" },
    { 6, "IR\002\000\000\002" }
  },
  {
    "CD",
    N_("Print to CD"),
    1,
    0,
    0,
    0,
    { 36, "PM\002\000\000\000IR\002\000\000\001EX\006\000\000\000\000\000\005\000FP\003\000\000\000\000PP\003\000\000\002\001" },
    { 6, "IR\002\000\000\000"}
  },
};

DECLARE_INPUT_SLOT(r1800);

static const input_slot_t rx700_input_slots[] =
{
  {
    "Rear",
    N_("Rear Tray"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\001\000" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "Front",
    N_("Front Tray"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\001\001" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "CD",
    N_("Print to CD"),
    1,
    0,
    0,
    0,
    { 36, "PM\002\000\000\000IR\002\000\000\001EX\006\000\000\000\000\000\005\000FP\003\000\000\000\000PP\003\000\000\002\001" },
    { 6, "IR\002\000\000\000"}
  },
  {
    "PhotoBoard",
    N_("Photo Board"),
    0,
    0,
    0,
    0,
    { 23, "IR\002\000\000\001EX\006\000\000\000\000\000\005\000PP\003\000\000\002\000" },
    { 6, "IR\002\000\000\000"}
  },
};

DECLARE_INPUT_SLOT(rx700);

static const input_slot_t pro_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    0,
    0,
    { 7, "PP\003\000\000\002\000" },
    { 0, "" }
  },
  {
    "Roll",
    N_("Roll Feed"),
    0,
    1,
    0,
    0,
    { 7, "PP\003\000\000\003\000" },
    { 0, "" }
  }
};

DECLARE_INPUT_SLOT(pro_roll_feed);

static const input_slot_t spro5000_input_slots[] =
{
  {
    "CutSheet1",
    N_("Cut Sheet Bin 1"),
    0,
    0,
    0,
    0,
    { 7, "PP\003\000\000\001\001" },
    { 0, "" }
  },
  {
    "CutSheet2",
    N_("Cut Sheet Bin 2"),
    0,
    0,
    0,
    0,
    { 7, "PP\003\000\000\002\001" },
    { 0, "" }
  },
  {
    "CutSheetAuto",
    N_("Cut Sheet Autoselect"),
    0,
    0,
    0,
    0,
    { 7, "PP\003\000\000\001\377" },
    { 0, "" }
  },
  {
    "ManualSelect",
    N_("Manual Selection"),
    0,
    0,
    0,
    0,
    { 7, "PP\003\000\000\002\001" },
    { 0, "" }
  }
};

DECLARE_INPUT_SLOT(spro5000);

static const input_slot_t b500_input_slots[] =
{
  {
    "Rear",
    N_("Rear Tray"),
    0,
    0,
    DUPLEX_TUMBLE,
    0,
    { 7, "PP\003\000\000\001\000" },
    { 0, "" }
  },
  {
    "Front",
    N_("Front Tray"),
    0,
    0,
    DUPLEX_TUMBLE,
    0,
    { 7, "PP\003\000\000\001\001" },
    { 0, "" }
  }
};

DECLARE_INPUT_SLOT(b500);

static const input_slot_list_t default_input_slot_list =
{
  "Standard",
  NULL,
  0,
};

typedef struct
{
  const char *name;
  const input_slot_list_t *input_slots;
} inslot_t;

static const inslot_t the_slots[] =
{
  { "cd_cutter_roll_feed", &cd_cutter_roll_feed_input_slot_list },
  { "cd_roll_feed", &cd_roll_feed_input_slot_list },
  { "cutter_roll_feed", &cutter_roll_feed_input_slot_list },
  { "default", &default_input_slot_list },
  { "pro_roll_feed", &pro_roll_feed_input_slot_list },
  { "r1800", &r1800_input_slot_list },
  { "r2400", &r2400_input_slot_list },
  { "rx700", &rx700_input_slot_list },
  { "spro5000", &spro5000_input_slot_list },
  { "b500", &b500_input_slot_list },
  { "standard_roll_feed", &standard_roll_feed_input_slot_list },
};

const input_slot_list_t *
stpi_escp2_get_input_slot_list_named(const char *n)
{
  int i;
  if (n)
    {
      for (i = 0; i < sizeof(the_slots) / sizeof(inslot_t); i++)
	{
	  if (strcmp(n, the_slots[i].name) == 0)
	    return the_slots[i].input_slots;
	}
      stp_erprintf("Cannot find input slot list named %s\n", n);
    }
  return NULL;
}
