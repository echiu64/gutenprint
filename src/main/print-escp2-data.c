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
#include <limits.h>
#include <assert.h>

typedef struct
{
  const char *attr_name;
  short bit_shift;
  short bit_width;
} escp2_printer_attr_t;

static const escp2_printer_attr_t escp2_printer_attrs[] =
{
  { "command_mode",		0, 4 },
  { "zero_margin",		4, 2 },
  { "variable_mode",		6, 1 },
  { "graymode",		 	7, 1 },
  { "fast_360",			8, 1 },
  { "send_zero_advance",        9, 1 },
  { "supports_ink_change",     10, 1 },
  { "packet_mode",             11, 1 },
  { "interchangeable_ink",     12, 1 },
  { "envelope_landscape",      13, 1 },
};

static stpi_escp2_printer_t escp2_model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    "standard"
  },
  /* 1: Stylus Color 400/500 */
  {
    "standard"
  },
  /* 2: Stylus Color 1500 */
  {
    "cmy"
  },
  /* 3: Stylus Color 600 */
  {
    "standard"
  },
  /* 4: Stylus Color 800 */
  {
    "standard"
  },
  /* 5: Stylus Color 850 */
  {
    "standard"
  },
  /* 6: Stylus Color 1520 */
  {
    "standard"
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    "photo_gen1"
  },
  /* 8: Stylus Photo EX */
  {
    "photo_gen1"
  },
  /* 9: Stylus Photo */
  {
    "photo_gen1"
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  {
    "standard"
  },
  /* 11: Stylus Color 640 */
  {
    "standard"
  },
  /* 12: Stylus Color 740/Stylus Scan 2000/Stylus Scan 2500 */
  {
    "standard"
  },
  /* 13: Stylus Color 900 */
  {
    "standard"
  },
  /* 14: Stylus Photo 750 */
  {
    "photo_gen1"
  },
  /* 15: Stylus Photo 1200 */
  {
    "photo_gen1"
  },
  /* 16: Stylus Color 860 */
  {
    "standard"
  },
  /* 17: Stylus Color 1160 */
  {
    "standard"
  },
  /* 18: Stylus Color 660 */
  {
    "standard"
  },
  /* 19: Stylus Color 760 */
  {
    "standard"
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    "photo_gen1"
  },
  /* 21: Stylus Color 480 */
  {
    "x80"
  },
  /* 22: Stylus Photo 870/875 */
  {
    "photo_gen2"
  },
  /* 23: Stylus Photo 1270 */
  {
    "photo_gen2"
  },
  /* 24: Stylus Color 3000 */
  {
    "standard"
  },
  /* 25: Stylus Color 670 */
  {
    "standard"
  },
  /* 26: Stylus Photo 2000P */
  {
    "photo_pigment"
  },
  /* 27: Stylus Pro 5000 */
  {
    "pro_gen1"
  },
  /* 28: Stylus Pro 7000 */
  {
    "pro_gen1"
  },
  /* 29: Stylus Pro 7500 */
  {
    "pro_pigment"
  },
  /* 30: Stylus Pro 9000 */
  {
    "pro_gen1"
  },
  /* 31: Stylus Pro 9500 */
  {
    "pro_pigment"
  },
  /* 32: Stylus Color 777/680 */
  {
    "standard"
  },
  /* 33: Stylus Color 880/83/C60 */
  {
    "standard"
  },
  /* 34: Stylus Color 980 */
  {
    "standard"
  },
  /* 35: Stylus Photo 780/790 */
  {
    "photo_gen2"
  },
  /* 36: Stylus Photo 785/890/895/915/935 */
  {
    "photo_gen2"
  },
  /* 37: Stylus Photo 1280/1290 */
  {
    "photo_gen2"
  },
  /* 38: Stylus Color 580 */
  {
    "x80"
  },
  /* 39: Stylus Color Pro XL */
  {
    "standard"
  },
  /* 40: Stylus Pro 5500 */
  {
    "pro_pigment"
  },
  /* 41: Stylus Pro 10000 */
  {
    "pro_gen2"
  },
  /* 42: Stylus C20SX/C20UX */
  {
    "x80"
  },
  /* 43: Stylus C40SX/C40UX/C41SX/C41UX/C42SX/C42UX */
  {
    "x80"
  },
  /* 44: Stylus C70/C80 */
  {
    "c80"
  },
  /* 45: Stylus Color Pro */
  {
    "standard"
  },
  /* 46: Stylus Photo 950/960 */
  {
    "f360_photo"
  },
  /* 47: Stylus Photo 2100/2200 */
  {
    "f360_ultrachrome"
  },
  /* 48: Stylus Pro 7600 */
  {
    "pro_ultrachrome"
  },
  /* 49: Stylus Pro 9600 */
  {
    "pro_ultrachrome"
  },
  /* 50: Stylus Photo 825/830 */
  {
    "photo_gen2"
  },
  /* 51: Stylus Photo 925 */
  {
    "photo_gen2"
  },
  /* 52: Stylus Color C62 */
  {
    "standard"
  },
  /* 53: Japanese PM-950C */
  {
    "f360_photo7_japan"
  },
  /* 54: Stylus Photo EX3 */
  {
    "photo_gen1"
  },
  /* 55: Stylus C82/CX-5200 */
  {
    "c82"
  },
  /* 56: Stylus C50 */
  {
    "x80"
  },
  /* 57: Japanese PM-970C */
  {
    "f360_photo7_japan"
  },
  /* 58: Japanese PM-930C */
  {
    "photo_gen2"
  },
  /* 59: Stylus C43SX/C43UX/C44SX/C44UX (WRONG -- see 43!) */
  {
    "x80"
  },
  /* 60: Stylus C84 */
  {
    "c82"
  },
  /* 61: Stylus Color C63/C64 */
  {
    "c64"
  },
  /* 62: Stylus Photo 900 */
  {
    "photo_gen2"
  },
  /* 63: Stylus Photo R300 */
  {
    "photo_gen3"
  },
  /* 64: PM-G800/Stylus Photo R800 */
  {
    "cmykrb"
  },
  /* 65: Stylus Photo CX4600 */
  {
    "cx3650"
  },
  /* 66: Stylus Color C65/C66 */
  {
    "c64"
  },
  /* 67: Stylus Photo R1800 */
  {
    "cmykrb"
  },
  /* 68: PM-G820 */
  {
    "cmykrb"
  },
  /* 69: Stylus C86 */
  {
    "c82"
  },
  /* 70: Stylus Photo RX700 */
  {
    "photo_gen4"
  },
  /* 71: Stylus Photo R2400 */
  {
    "f360_ultrachrome_k3"
  },
  /* 72: Stylus CX3700/3800/3810 */
  {
    "c64"
  },
  /* 73: E-100/PictureMate */
  {
    "picturemate_6"
  },
  /* 74: PM-A650 */
  {
    "c64"
  },
  /* 75: Japanese PM-A750 */
  {
    "c64"
  },
  /* 76: Japanese PM-A890 */
  {
    "photo_gen4"
  },
  /* 77: Japanese PM-D600 */
  {
    "c64"
  },
  /* 78: Stylus Photo 810/820 */
  {
    "photo_gen2"
  },
  /* 79: Stylus CX6400 */
  {
    "c82"
  },
  /* 80: Stylus CX6600 */
  {
    "c82"
  },
  /* 81: Stylus Photo R260 */
  {
    "claria"
  },
  /* 82: Stylus Photo 1400 */
  {
    "claria"
  },
  /* 83: Stylus Photo R240 */
  {
    "photo_gen3_4"
  },
  /* 84: Stylus Photo RX500 */
  {
    "photo_gen3"
  },
  /* 85: Stylus C120 */
  {
    "c120"
  },
  /* 86: PictureMate 4-color */
  {
    "picturemate_4"
  },
  /* 87: B-500DN */
  {
    "b500"
  },
};

static const int escp2_model_limit =
sizeof(escp2_model_capabilities) / sizeof(stpi_escp2_printer_t);

static
load_model_from_file(const stp_vars_t *v, stp_mxml_node_t *xmod, int model)
{
  stp_mxml_node_t *tmp = xmod->child;
  stpi_escp2_printer_t *p = &(escp2_model_capabilities[model]);
  int found_black_head_config = 0;
  int found_fast_head_config = 0;
  p->max_black_resolution = -1;
  p->cd_x_offset = -1;
  p->cd_y_offset = -1;
  while (tmp)
    {
      if (tmp->type == STP_MXML_ELEMENT)
	{
	  const char *name = tmp->value.element.name;
	  const char *target = stp_mxmlElementGetAttr(tmp, "href");
	  if (target)
	    {
	      if (!strcmp(name, "media"))
		stp_escp2_load_media(v, target);
	      else if (!strcmp(name, "inputSlots"))
		stp_escp2_load_input_slots(v, target);
	      else if (!strcmp(name, "mediaSizes"))
		stp_escp2_load_media_sizes(v, target);
	      else if (!strcmp(name, "printerWeaves"))
		stp_escp2_load_printer_weaves(v, target);
	      else if (!strcmp(name, "qualityPresets"))
		stp_escp2_load_quality_presets(v, target);
	      else if (!strcmp(name, "resolutions"))
		stp_escp2_load_resolutions(v, target);
	    }
	  else if (tmp->child && tmp->child->type == STP_MXML_TEXT)
	    {
	      const char *val = tmp->child->value.text.string;
	      if (!strcmp(name, "verticalBorderlessSequence"))
		p->vertical_borderless_sequence = stp_xmlstrtoraw(val);

	      else if (!strcmp(name, "preinitSequence"))
		p->preinit_sequence = stp_xmlstrtoraw(val);

	      else if (!strcmp(name, "preinitRemoteSequence"))
		p->preinit_remote_sequence = stp_xmlstrtoraw(val);

	      else if (!strcmp(name, "postinitRemoteSequence"))
		p->postinit_remote_sequence = stp_xmlstrtoraw(val);
	      else if (!strcmp(name, "commandSet"))
		{
		  if (!strcmp(val, "1998"))
		    p->flags |= MODEL_COMMAND_1998;
		  else if (!strcmp(val, "1999"))
		    p->flags |= MODEL_COMMAND_1999;
		  else if (!strcmp(val, "2000"))
		    p->flags |= MODEL_COMMAND_2000;
		  else if (!strcmp(val, "Pro"))
		    p->flags |= MODEL_COMMAND_PRO;
		}
	      else if (!strcmp(name, "borderless"))
		{
		  if (!strcmp(val, "No"))
		    p->flags |= MODEL_ZEROMARGIN_NO;
		  else if (!strcmp(val, "Yes"))
		    p->flags |= MODEL_ZEROMARGIN_YES;
		  else if (!strcmp(val, "Full"))
		    p->flags |= MODEL_ZEROMARGIN_FULL;
		  else if (!strcmp(val, "HorizontalOnly"))
		    p->flags |= MODEL_ZEROMARGIN_H_ONLY;
		}
	      else if (!strcmp(name, "preferredEnvelopeOrientation") &&
		       !strcmp(val, "Landscape"))
		p->flags |= MODEL_ENVELOPE_LANDSCAPE_YES;
	      else if (!strcmp(name, "headConfiguration"))
		{
		  const char *htype = stp_mxmlElementGetAttr(tmp, "type");
		  unsigned long data[4] = { 0, 0, 0, 0 };
		  stp_mxml_node_t *child = tmp->child;
		  while (child)
		    {
		      if (child->type == STP_MXML_ELEMENT && child->child &&
			  child->child->type == STP_MXML_TEXT)
			{
			  const char *cname = child->value.element.name;
			  const char *cval = child->child->value.text.string;
			  if (!strcmp(cname, "Nozzles"))
			    data[0] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "MinNozzles"))
			    data[1] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "FirstNozzle"))
			    data[2] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "NozzleSeparation"))
			    data[3] = stp_xmlstrtoul(cval);
			}
		      child = child->next;
		    }		      
		  if (!strcmp(htype, "default"))
		    {
		      p->nozzles = data[0];
		      p->min_nozzles = data[1];
		      p->nozzle_start = data[2];
		      p->nozzle_separation = data[3];
		      if (!found_black_head_config)
			{
			  p->black_nozzles = data[0];
			  p->min_black_nozzles = data[1];
			  p->black_nozzle_start = data[2];
			  p->black_nozzle_separation = data[3];
			}
		      if (!found_fast_head_config)
			{
			  p->fast_nozzles = data[0];
			  p->min_fast_nozzles = data[1];
			  p->fast_nozzle_start = data[2];
			  p->fast_nozzle_separation = data[3];
			}
		    }
		  else if (!strcmp(htype, "black"))
		    {
		      p->black_nozzles = data[0];
		      p->min_black_nozzles = data[1];
		      p->black_nozzle_start = data[2];
		      p->black_nozzle_separation = data[3];
		      found_black_head_config = 1;
		    }
		  else if (!strcmp(htype, "fast"))
		    {
		      p->fast_nozzles = data[0];
		      p->min_fast_nozzles = data[1];
		      p->fast_nozzle_start = data[2];
		      p->fast_nozzle_separation = data[3];
		      found_fast_head_config = 1;
		    }
		}
	      else if (!strcmp(name, "margins"))
		{
		  const char *itype = stp_mxmlElementGetAttr(tmp, "interleave");
		  const char *mtype = stp_mxmlElementGetAttr(tmp, "media");
		  unsigned long data[4];
		  int i = 0;
		  stp_mxml_node_t *child = tmp->child;
		  while (child && i < 4)
		    {
		      if (child->type == STP_MXML_TEXT)
			data[i++] = stp_xmlstrtoul(child->value.text.string);
		      child = child->next;
		    }		      
		  if (itype && !strcmp(itype, "soft") &&
		      mtype && !strcmp(mtype, "sheet"))
		    {
		      p->left_margin = data[0];
		      p->right_margin = data[1];
		      p->top_margin = data[2];
		      p->bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "printer") &&
			   mtype && !strcmp(mtype, "sheet"))
		    {
		      p->m_left_margin = data[0];
		      p->m_right_margin = data[1];
		      p->m_top_margin = data[2];
		      p->m_bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "soft") &&
			   mtype && !strcmp(mtype, "roll"))
		    {
		      p->roll_left_margin = data[0];
		      p->roll_right_margin = data[1];
		      p->roll_top_margin = data[2];
		      p->roll_bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "printer") &&
			   mtype && !strcmp(mtype, "roll"))
		    {
		      p->m_roll_left_margin = data[0];
		      p->m_roll_right_margin = data[1];
		      p->m_roll_top_margin = data[2];
		      p->m_roll_bottom_margin = data[3];
		    }
		}
	      else if (!strcmp(name, "physicalChannels"))
		p->physical_channels = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "baseSeparation"))
		p->base_separation = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "resolutionScale"))
		p->resolution_scale = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "maxBlackResolution"))
		p->max_black_resolution = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minimumResolution"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->min_hres = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->min_vres = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumResolution"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->max_hres = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_vres = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "extraVerticalFeed"))
		p->extra_feed = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "separationRows"))
		p->separation_rows = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "pseudoSeparationRows"))
		p->pseudo_separation_rows = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "zeroMarginOffset"))
		p->zero_margin_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "microLeftMargin"))
		p->micro_left_margin = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "initialVerticalOffset"))
		p->initial_vertical_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "blackInitialVerticalOffset"))
		p->black_initial_vertical_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "extra720DPISeparation"))
		p->extra_720dpi_separation = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minHorizontalAlignment"))
		p->min_horizontal_position_alignment = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "baseHorizontalAlignment"))
		p->base_horizontal_position_alignment = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "bidirectionalAutoUpperLimit"))
		p->bidirectional_upper_limit = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minimumMediaSize"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->min_paper_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->min_paper_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumMediaSize"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->max_paper_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_paper_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumImageableArea"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->max_imageable_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_imageable_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "CDOffset"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->cd_x_offset = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->cd_y_offset = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "CDMediaSize"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->cd_page_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->cd_page_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "extraBottom"))
		p->paper_extra_bottom = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "AlignmentChoices"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->alignment_passes =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alignment_choices =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alternate_alignment_passes =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alternate_alignment_choices =
		    stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "ChannelNames"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->channel_names = stp_string_list_create();
		  while (child)
		    {
		      if (child->type == STP_MXML_ELEMENT &&
			  !strcmp(child->value.element.name, "ChannelName"))
			{
			  const char *cname = stp_mxmlElementGetAttr(child, "name");
			  stp_string_list_add_string(p->channel_names, cname, cname);
			}
		      child = child->next;
		    }
		}
	    }
	  else
	    {
	      if (!strcmp(name, "supportsVariableDropsizes"))
		p->flags |= MODEL_VARIABLE_YES;
	      else if (!strcmp(name, "hasFastGraymode"))
		p->flags |= MODEL_GRAYMODE_YES;
	      else if (!strcmp(name, "hasFast360DPI"))
		p->flags |= MODEL_FAST_360_YES;
	      else if (!strcmp(name, "sendZeroAdvance"))
		p->flags |= MODEL_SEND_ZERO_ADVANCE_YES;
	      else if (!strcmp(name, "supportsInkChange"))
		p->flags |= MODEL_SUPPORTS_INK_CHANGE_YES;
	      else if (!strcmp(name, "supportsD4Mode"))
		p->flags |= MODEL_PACKET_MODE_YES;
	      else if (!strcmp(name, "hasInterchangeableInkCartridges"))
		p->flags |= MODEL_INTERCHANGEABLE_INK_YES;
	    }
	}
      tmp = tmp->next;
    }
}

void
stp_escp2_load_model(const stp_vars_t *v, int model)
{
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  char buf[1024];
  int found = 0;

  stp_xml_init();
  sprintf(buf, "escp2/model/model_%d.xml", model);
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *fn = stpi_path_merge(dn, buf);
      stp_mxml_node_t *doc = stp_mxmlLoadFromFile(NULL, fn, STP_MXML_NO_CALLBACK);
      stp_free(fn);
      if (doc)
	{
	  stp_mxml_node_t *xmod =
	    stp_mxmlFindElement(doc, doc, "escp2:model", NULL, NULL,
				STP_MXML_DESCEND);
	  if (xmod)
	    {
	      const char *stmp = stp_mxmlElementGetAttr(xmod, "id");
	      assert(stmp && stp_xmlstrtol(stmp) == model);
	      if (stmp && stp_xmlstrtol(stmp) == model)
		{
		  load_model_from_file(v, xmod, model);
		  found = 1;
		}
	    }
	  stp_mxmlDelete(doc);
	  if (found)
	    break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  if (! found)
    {
      stp_erprintf("Unable to find printer definition for model %d!\n", model);
      stp_abort();
    }
}

static int printer_is_loading = 0;

stpi_escp2_printer_t *
stp_escp2_get_printer(const stp_vars_t *v)
{
  int model = stp_get_model_id(v);
  if (model < 0 || model >= escp2_model_limit)
    {
      stp_erprintf("Unable to find printer definition for model %d!\n", model);
      stp_abort();
    }
  if (!printer_is_loading && ! escp2_model_capabilities[model].media)
    {
      printer_is_loading = 1;
      stp_escp2_load_model(v, model);
      printer_is_loading = 0;
    }
  return &(escp2_model_capabilities[model]);
}

model_featureset_t
stp_escp2_get_cap(const stp_vars_t *v, escp2_model_option_t feature)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  model_featureset_t featureset =
    (((1ul << escp2_printer_attrs[feature].bit_width) - 1ul) <<
     escp2_printer_attrs[feature].bit_shift);
  return printdef->flags & featureset;
}

int
stp_escp2_has_cap(const stp_vars_t *v, escp2_model_option_t feature,
		  model_featureset_t class)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  model_featureset_t featureset =
    (((1ul << escp2_printer_attrs[feature].bit_width) - 1ul) <<
     escp2_printer_attrs[feature].bit_shift);
  return ((printdef->flags & featureset) == class);
}
