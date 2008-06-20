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
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include "print-escp2.h"

static const res_t r_360x90dpi =
{ "360x90dpi",     N_("360 x 90 DPI Fast Economy"),
  360,  90,  360,  90,  360,  90,   0, 0, 1 };
static const res_t r_360x90sw =
{ "360x90sw",      N_("360 x 90 DPI Fast Economy"),
  360,  90,  360,  90,  360,  90,   1, 0, 1 };

static const res_t r_360x120dpi =
{ "360x120dpi",    N_("360 x 120 DPI Economy"),
  360,  120,  360,  120,  360,  120,  0, 0, 1 };
static const res_t r_360x120sw =
{ "360x120sw",     N_("360 x 120 DPI Economy"),
  360,  120,  360,  120,  360,  120,  1, 0, 1 };

static const res_t r_180dpi =
{ "180dpi",        N_("180 DPI Economy"),
  180,  180,  180,  180,  180,  180,  0, 0, 1 };
static const res_t r_180sw =
{ "180sw",         N_("180 DPI Economy"),
  180,  180,  180,  180,  180,  180,  1, 0, 1 };

static const res_t r_360x180dpi =
{ "360x180dpi",    N_("360 x 180 DPI Draft"),
  360,  180,  360,  180,  360,  180,  0, 0, 1 };
static const res_t r_360x180sw =
{ "360x180sw",     N_("360 x 180 DPI Draft"),
  360,  180,  360,  180,  360,  180,  1, 0, 1 };

static const res_t r_360x240dpi =
{ "360x240dpi",    N_("360 x 240 DPI Draft"),
  360,  240,  360,  240,  360,  240,  0, 0, 1 };
static const res_t r_360x240sw =
{ "360x240sw",     N_("360 x 240 DPI Draft"),
  360,  240,  360,  240,  360,  240,  1, 0, 1 };

static const res_t r_360mw =
{ "360mw",         N_("360 DPI High Quality"),
  360,  360,  360,  360,  360,  360,  0, 1, 1 };
static const res_t r_360pro =
{ "360pro",        N_("360 DPI"),
  360,  360,  360,  360,  360,  360,  0, 1, 1 };
static const res_t r_360 =
{ "360",           N_("360 DPI"),
  360,  360,  360,  360,  360,  360,  0, 0, 1 };
static const res_t r_360sw =
{ "360sw",         N_("360 DPI"),
  360,  360,  360,  360,  360,  360,  1, 0, 1 };

static const res_t r_720x360mw =
{ "720x360mw",     N_("720 x 360 DPI"),
  720,  360,  720,  360,  720,  360,  0, 1, 1 };
static const res_t r_720x360sw =
{ "720x360sw",     N_("720 x 360 DPI"),
  720,  360,  720,  360,  720,  360,  1, 0, 1 };
static const res_t r_720x360un =
{ "720x360un",     N_("720 x 360 DPI Enhanced"),
  720,  360, 1440,  720,  720,  360,  1, 0, 1 };

static const res_t r_720mw =
{ "720mw",         N_("720 DPI"),
  720,  720,  720,  720,  720,  720,  0, 1, 1 };
static const res_t r_720sw =
{ "720sw",         N_("720 DPI"),
  720,  720,  720,  720,  720,  720,  1, 0, 1 };
static const res_t r_720un =
{ "720un",         N_("720 DPI High Quality"),
  720,  720, 1440,  720,  720,  720,  1, 0, 1 };
static const res_t r_720hq =
{ "720hq",         N_("720 DPI High Quality"),
  720,  720,  720,  720,  720,  720,  1, 0, 2 };
static const res_t r_720hq2 =
{ "720hq2",        N_("720 DPI Highest Quality"),
  720,  720,  720,  720,  720,  720,  1, 0, 4 };
static const res_t r_720x720oov =
{ "720x720oov",    N_("720 x 720 DPI"),
  2880, 720,  2880, 720,  720, 720,  1, 0, 1 };

static const res_t r_1440x720mw =
{ "1440x720mw",     N_("1440 x 720 DPI"),
  1440, 720,  1440, 720,  1440, 720,  0, 1, 1 };
static const res_t r_1440x720sw =
{ "1440x720sw",    N_("1440 x 720 DPI"),
  1440, 720,  1440, 720,  1440, 720,  1, 0, 1 };
static const res_t r_1440x720hq2 =
{ "1440x720hq2",   N_("1440 x 720 DPI Highest Quality"),
  1440, 720,  1440, 720,  1440, 720,  1, 0, 2 };
static const res_t r_720x1440sw =
{ "720x1440sw",    N_("1440 x 720 DPI Transposed"),
  720, 1440,  720, 1440,  720, 1440,  1, 0, 1 };
static const res_t r_720x1440ov =
{ "720x1440ov",   N_("1440 x 720 DPI Transposed"),
  1440, 1440, 1440, 1440, 720, 1440,  1, 0, 1};
static const res_t r_1440x720ov =
{ "1440x720ov",    N_("1440 x 720 DPI"),
  2880, 720,  2880, 720,  1440, 720,  1, 0, 1 };

static const res_t r_2880x720mw =
{ "2880x720mw",    N_("2880 x 720 DPI"),
  2880, 720,  2880, 720,  2880, 720,  0, 1, 1};
static const res_t r_2880x720sw =
{ "2880x720sw",    N_("2880 x 720 DPI"),
  2880, 720,  2880, 720,  2880, 720,  1, 0, 1};
static const res_t r_2880x720hq2 =
{ "2880x720hq2",   N_("2880 x 720 DPI Highest Quality"),
  2880, 720,  2880, 720,  2880, 720,  1, 0, 2 };

static const res_t r_1440x1440mw =
{ "1440x1440mw",   N_("1440 x 1440 DPI"),
  1440, 1440,  1440, 1440,  1440, 1440, 0, 1, 1};
static const res_t r_1440x1440sw =
{ "1440x1440sw",   N_("1440 x 1440 DPI"),
  1440, 1440,  1440, 1440,  1440, 1440, 1, 0, 1};
static const res_t r_1440x1440ov =
{ "1440x1440ov",   N_("1440 x 1440 DPI"),
  2880, 1440,  2880, 1440,  1440, 1440, 1, 0, 1};

static const res_t r_2880x1440mw =
{ "2880x1440mw",   N_("2880 x 1440 DPI"),
  2880, 1440,  2880, 1440,  2880, 1440, 0, 1, 1};
static const res_t r_2880x1440sw =
{ "2880x1440sw",   N_("2880 x 1440 DPI"),
  2880, 1440,  2880, 1440,  2880, 1440, 1, 0, 1};
static const res_t r_1440x2880sw =
{ "1440x2880sw",   N_("2880 x 1440 DPI Transposed"),
  1440, 2880,  1440, 2880,  1440, 2880, 1, 0, 1};
static const res_t r_2880x1440sw2400 =
{ "2880x1440sw",   N_("2880 x 1440 DPI"),
  1440, 2880,  1440, 2880,  1440, 2880, 1, 0, 1};

static const res_t r_5760x1440sw =
{ "5760x1440sw",   N_("5760 x 1440 DPI"),
  5760, 1440,  5760, 1440,  5760, 1440, 1, 0, 1};

static const res_t r_2880x2880mw =
{ "2880x2880mw",   N_("2880 x 2880 DPI"),
  2880, 2880,  2880, 2880,  2880, 2880, 0, 1, 1};
static const res_t r_2880x2880sw =
{ "2880x2880sw",   N_("2880 x 2880 DPI"),
  2880, 2880,  2880, 2880,  2880, 2880, 1, 0, 1};

static const res_t r_5760x2880mw =
{ "5760x2880mw",   N_("5760 x 2880 DPI"),
  5760, 2880,  5760, 2880,  5760, 2880, 0, 1, 1};
static const res_t r_5760x2880sw =
{ "5760x2880sw",   N_("5760 x 2880 DPI"),
  5760, 2880,  5760, 2880,  5760, 2880, 1, 0, 1};



static const res_t *const stpi_escp2_720dpi_reslist[] =
{
  &r_360x90dpi,

  &r_360x120dpi,

  &r_180dpi,

  &r_360x240dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360,

  &r_720x360sw,

  &r_720mw,

  NULL
};

static const res_t *const stpi_escp2_1440dpi_reslist[] =
{
  &r_360x90sw,

  &r_360x120sw,

  &r_180sw,

  &r_360x240sw,

  &r_360x180sw,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,
  &r_720hq,

  &r_1440x720sw,
  &r_1440x720hq2,

  NULL
};

static const res_t *const stpi_escp2_2880dpi_reslist[] =
{
  &r_360x90sw,

  &r_360x120sw,

  &r_180sw,

  &r_360x240sw,

  &r_360x180sw,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,

  &r_1440x720sw,
  &r_1440x720hq2,

  &r_2880x720sw,
  &r_2880x720hq2,

  NULL
};

static const res_t *const stpi_escp2_2880_1440dpi_reslist[] =
{
  &r_360x90sw,

  &r_360x120sw,

  &r_180sw,

  &r_360x240sw,

  &r_360x180sw,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,

  &r_1440x720sw,

  &r_2880x720sw,

  &r_1440x1440sw,

  &r_2880x1440sw,

  &r_2880x2880sw,

  NULL
};

static const res_t *const stpi_escp2_g3_reslist[] =
{
  &r_360x90dpi,

  &r_360x120dpi,

  &r_180dpi,

  &r_360x240dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360,

  &r_720x360sw,

  &r_720sw,
  &r_720hq,

  &r_1440x720sw,
  &r_1440x720hq2,

  NULL
};

static const res_t *const stpi_escp2_superfine_reslist[] =
{
  &r_360x90sw,

  &r_360x120sw,

  &r_360x180sw,

  &r_360x240sw,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,

  &r_1440x720sw,

  &r_1440x1440ov,

  &r_2880x1440sw,

  &r_5760x1440sw,

  &r_2880x2880sw,

  &r_5760x2880sw,

  NULL
};

static const res_t *const stpi_escp2_claria_1400_reslist[] =
{
  &r_360x90sw,

  &r_360x180sw,

  &r_360x240sw,

  &r_360sw,

  &r_720x360sw,

  &r_720x360un,

  &r_720sw,

  &r_720un,

  &r_1440x720sw,

  &r_1440x1440ov,

  &r_2880x1440sw,

  &r_5760x1440sw,

  &r_2880x2880sw,

  &r_5760x2880sw,

  NULL
};

static const res_t *const stpi_escp2_picturemate_reslist[] =
{

  &r_720x720oov,

  &r_1440x720ov,

  &r_720x1440ov,

  &r_1440x1440sw,

  &r_2880x1440sw,

  &r_5760x1440sw,

  NULL
};

static const res_t *const stpi_escp2_sc500_reslist[] =
{
  &r_360x90dpi,

  &r_360x120dpi,

  &r_180dpi,

  &r_360x240dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360,

  &r_720x360mw,

  &r_720mw,

  NULL
};

static const res_t *const stpi_escp2_g3_720dpi_reslist[] =
{
  &r_360x90dpi,

  &r_360x120sw,

  &r_180dpi,

  &r_360x240sw,

  &r_360x180dpi,

  &r_360mw,
  &r_360,

  &r_720x360sw,

  &r_720sw,
  &r_720hq,
  &r_720hq2,

  NULL
};

static const res_t *const stpi_escp2_720dpi_soft_reslist[] =
{
  &r_360x90dpi,

  &r_360x120sw,

  &r_180dpi,

  &r_360x240sw,

  &r_360x180dpi,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,
  &r_720hq,
  &r_720hq2,

  NULL
};

static const res_t *const stpi_escp2_sc640_reslist[] =
{
  &r_360x90dpi,

  &r_180dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360,

  &r_720x360sw,

  &r_720mw,

  &r_1440x720sw,
  &r_1440x720hq2,

  NULL
};

static const res_t *const stpi_escp2_pro_reslist[] =
{
  &r_360x90dpi,

  &r_180dpi,

  &r_360x180dpi,

  &r_360pro,

  &r_720x360mw,

  &r_720mw,

  &r_1440x720mw,

  &r_2880x720mw,

  &r_1440x1440mw,

  &r_2880x1440mw,

  &r_2880x2880mw,

  NULL
};

typedef struct
{
  const char *name;
  const res_t *const *const res_list;
} resolution_t;

static const resolution_t the_resolutions[] =
{
  { "superfine", stpi_escp2_superfine_reslist },
  { "claria_1400", stpi_escp2_claria_1400_reslist },
  { "pro", stpi_escp2_pro_reslist },
  { "720dpi", stpi_escp2_720dpi_reslist },
  { "720dpi_soft", stpi_escp2_720dpi_soft_reslist },
  { "g3_720dpi", stpi_escp2_g3_720dpi_reslist },
  { "1440dpi", stpi_escp2_1440dpi_reslist },
  { "2880dpi", stpi_escp2_2880dpi_reslist },
  { "2880_1440dpi", stpi_escp2_2880_1440dpi_reslist },
  { "g3", stpi_escp2_g3_reslist },
  { "sc500", stpi_escp2_sc500_reslist },
  { "sc640", stpi_escp2_sc640_reslist },
  { "picturemate", stpi_escp2_picturemate_reslist },
};

const res_t *const *
stpi_escp2_get_reslist_named(const char *n)
{
  int i;
  if (n)
    {
      for (i = 0; i < sizeof(the_resolutions) / sizeof(resolution_t); i++)
	{
	  if (strcmp(n, the_resolutions[i].name) == 0)
	    return the_resolutions[i].res_list;
	}
      stp_erprintf("Cannot find resolution list named %s\n", n);
    }
  return NULL;
}

int
stp_escp2_load_printer_weaves(const stp_vars_t *v, const char *name)
{
  int model = stp_get_model_id(v);
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *weaves =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (weaves)
	{
	  stp_mxml_node_t *node = stp_mxmlFindElement(weaves, weaves,
						      "escp2:PrinterWeaves", NULL,
						      NULL, STP_MXML_DESCEND);
	  if (node)
	    {
	      printer_weave_list_t *xpw = stp_malloc(sizeof(printer_weave_list_t));
	      int count = 0;
	      stp_mxml_node_t *child = node->child;
	      while (child)
		{
		  if (child->type == STP_MXML_ELEMENT &&
		      !strcmp(child->value.element.name, "weave"))
		    count++;
		  child = child->next;
		}
	      stpi_escp2_model_capabilities[model].printer_weaves = xpw;
	      if (stp_mxmlElementGetAttr(node, "name"))
		xpw->name = stp_strdup(stp_mxmlElementGetAttr(node, "name"));
	      else
		xpw->name = stp_strdup(name);
	      xpw->n_printer_weaves = count;
	      xpw->printer_weaves = stp_zalloc(sizeof(printer_weave_t) * count);
	      child = node->child;
	      count = 0;
	      while (child)
		{
		  if (child->type == STP_MXML_ELEMENT &&
		      !strcmp(child->value.element.name, "weave"))
		    {
		      const char *wname = stp_mxmlElementGetAttr(child, "name");
		      const char *wtext = stp_mxmlElementGetAttr(child, "text");
		      const char *cmd = stp_mxmlElementGetAttr(child, "command");
		      if (wname)
			xpw->printer_weaves[count].name = stp_strdup(wname);
		      if (wtext)
			xpw->printer_weaves[count].text = stp_strdup(wtext);
		      if (cmd)
			xpw->printer_weaves[count].command =
			  stp_xmlstrtoraw(cmd);
		      count++;
		    }
		  child = child->next;
		}
	    }
	  stp_mxmlDelete(weaves);
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  if (! found)
    stp_eprintf(v, "Unable to load printer weaves for model %d (%s)!\n", model, name);
  return found;
}

int
stp_escp2_load_quality_presets(const stp_vars_t *v, const char *name)
{
  int model = stp_get_model_id(v);
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  int found = 0;
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *ffn = stpi_path_merge(dn, name);
      stp_mxml_node_t *qualities =
	stp_mxmlLoadFromFile(NULL, ffn, STP_MXML_NO_CALLBACK);
      stp_free(ffn);
      if (qualities)
	{
	  stp_mxml_node_t *node = stp_mxmlFindElement(qualities, qualities,
						      "escp2:QualityPresets", NULL,
						      NULL, STP_MXML_DESCEND);
	  if (node)
	    {
	      quality_list_t *qpw = stp_malloc(sizeof(quality_list_t));
	      int count = 0;
	      stp_mxml_node_t *child = node->child;
	      while (child)
		{
		  if (child->type == STP_MXML_ELEMENT &&
		      !strcmp(child->value.element.name, "quality"))
		    count++;
		  child = child->next;
		}
	      stpi_escp2_model_capabilities[model].quality_list = qpw;
	      if (stp_mxmlElementGetAttr(node, "name"))
		qpw->name = stp_strdup(stp_mxmlElementGetAttr(node, "name"));
	      else
		qpw->name = stp_strdup(name);
	      qpw->n_quals = count;
	      qpw->qualities = stp_zalloc(sizeof(quality_t) * count);
	      child = node->child;
	      count = 0;
	      while (child)
		{
		  if (child->type == STP_MXML_ELEMENT &&
		      !strcmp(child->value.element.name, "quality"))
		    {
		      stp_mxml_node_t *cchild = child->child;
		      const char *wname = stp_mxmlElementGetAttr(child, "name");
		      const char *wtext = stp_mxmlElementGetAttr(child, "text");
		      if (wname)
			qpw->qualities[count].name = stp_strdup(wname);
		      if (wtext)
			qpw->qualities[count].text = stp_strdup(wtext);
		      while (cchild)
			{
			  if (cchild->type == STP_MXML_ELEMENT &&
			      (!strcmp(cchild->value.element.name, "minimumResolution") ||
			       !strcmp(cchild->value.element.name, "maximumResolution") ||
			       !strcmp(cchild->value.element.name, "desiredResolution")))
			    {
			      long data[2] = { 0, 0 };
			      int i = 0;
			      stp_mxml_node_t *ccchild = cchild->child;
			      data[0] = stp_xmlstrtol(ccchild->value.text.string);
			      ccchild = ccchild->next;
			      data[1] = stp_xmlstrtol(ccchild->value.text.string);
			      if (!strcmp(cchild->value.element.name, "minimumResolution"))
				{
				  qpw->qualities[count].min_hres = data[0];
				  qpw->qualities[count].min_vres = data[1];
				}			      
			      else if (!strcmp(cchild->value.element.name, "maximumResolution"))
				{
				  qpw->qualities[count].max_hres = data[0];
				  qpw->qualities[count].max_vres = data[1];
				}			      
			      else if (!strcmp(cchild->value.element.name, "desiredResolution"))
				{
				  qpw->qualities[count].desired_hres = data[0];
				  qpw->qualities[count].desired_vres = data[1];
				}			      
			    }
			  cchild = cchild->next;
			}
		      count++;
		    }
		  child = child->next;
		}
	    }
	  stp_mxmlDelete(qualities);
	  found = 1;
	  break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  if (! found)
    stp_eprintf(v, "Unable to load quality presets for model %d (%s)!\n", model, name);
  return found;
}
