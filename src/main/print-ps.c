/*
 * "$Id$"
 *
 *   Print plug-in Adobe PostScript driver for the GIMP.
 *
 *   Copyright 1997-2002 Michael Sweet (mike@easysw.com) and
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/gutenprint-intl-internal.h>
#include "gutenprint-internal.h"
#include <time.h>
#include <string.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include "ppd.h"

#ifdef _MSC_VER
#define strncasecmp(s,t,n) _strnicmp(s,t,n)
#define strcasecmp(s,t) _stricmp(s,t)
#endif

/*
 * Local variables...
 */

static char *m_ppd_file = NULL;
static ppd_file_t *m_ppd = NULL;


/*
 * Local functions...
 */

static void	ps_hex(const stp_vars_t *, unsigned short *, int);
static void	ps_ascii85(const stp_vars_t *, unsigned short *, int, int);

static const stp_parameter_t the_parameters[] =
{
  {
    "PPDFile", N_("PPDFile"), N_("Basic Printer Setup"),
    N_("PPD File"),
    STP_PARAMETER_TYPE_FILE, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "ModelName", N_("Model Name"), N_("Basic Printer Setup"),
    N_("PPD File Model Name"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, -1, 0, 0
  },
};

static const int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

static int ps_option_to_param(stp_parameter_t *param, ppd_group_t *group, ppd_option_t *option)
{
  ppd_group_t *g, *grp = group;
  ppd_option_t *o;
  int i,j;

  if (grp == NULL)
  {
    for (i=0; i < m_ppd->num_groups; i++)
    {
      g = m_ppd->groups + i;
      for (j=0; j < g->num_options; j++)
      {
        o = g->options + j;
        if (strcasecmp(o->keyword, option->keyword) == 0)
        {
          grp = g;  /* found group for specified option */
          break;
        }
      }
    }
  }

  if (grp != NULL)
    param->category = grp->text;
  else
    param->category = NULL;

  param->name = option->keyword;
  param->text = option->text;
  param->help = option->text;
  switch (option->ui)
  {
    case PPD_UI_BOOLEAN:
      param->p_type = STP_PARAMETER_TYPE_BOOLEAN;
      break;
    case PPD_UI_PICKONE:
      default:
      param->p_type = STP_PARAMETER_TYPE_STRING_LIST;
      break;
  } 
  if (strcmp(param->name, "PageSize") == 0)
    param->p_class = STP_PARAMETER_CLASS_CORE;
  else
    param->p_class = STP_PARAMETER_CLASS_FEATURE;
  param->p_level = STP_PARAMETER_LEVEL_BASIC;
  param->is_mandatory = 1;
  param->is_active = 1;
  param->channel = -1;
  param->verify_this_parameter = 1;
  param->read_only = 0;

  return 0;
}

/*
 * 'ps_parameters()' - Return the parameter values for the given parameter.
 */

static int
check_ppd_file(const stp_vars_t *v)
{
  const char *ppd_file = stp_get_file_parameter(v, "PPDFile");

  if (ppd_file == NULL || ppd_file[0] == 0)
    {
      stp_dprintf(STP_DBG_PS, v, "Empty PPD file\n");
      return 0;
    }
  else if (m_ppd_file && strcmp(m_ppd_file, ppd_file) == 0)
    {
      stp_dprintf(STP_DBG_PS, v, "Not replacing PPD file %s\n", m_ppd_file);
      return 1;
    }
  else
    {
      stp_dprintf(STP_DBG_PS, v, "Replacing PPD file %s with %s\n",
		  m_ppd_file ? m_ppd_file : "(null)",
		  ppd_file ? ppd_file : "(null)");
      if (m_ppd != NULL)
	stpi_ppdClose(m_ppd);
      m_ppd = NULL;

      if (m_ppd_file)
	stp_free(m_ppd_file);
      m_ppd_file = NULL;

      if ((m_ppd = stpi_ppdOpenFile(ppd_file)) == NULL)
	{
	  stp_eprintf(v, "Unable to open PPD file %s\n", ppd_file);
	  return 0;
	}

      m_ppd_file = stp_strdup(ppd_file);
      return 1;
    }
}    
  

static stp_parameter_list_t
ps_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  ppd_group_t *group;	
  ppd_option_t *option;
  int i, j;
  int status = check_ppd_file(v);
  stp_dprintf(STP_DBG_PS, v, "Adding parameters from %s\n",
	      m_ppd_file ? m_ppd_file : "(null)");

  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));

  if (status)
    for (i=0; i < m_ppd->num_groups; i++)
      {
	group = m_ppd->groups + i;
	for (j=0; j < group->num_options; j++)
	  {
	    stp_parameter_t *param = stp_malloc(sizeof(stp_parameter_t));
	    option = group->options + j;
	    ps_option_to_param(param, group, option);
	    stp_dprintf(STP_DBG_PS, v, "Adding parameter %s %s\n",
			param->name, param->text);
	    stp_parameter_list_add_param(ret, param);
	  }
      }
  return ret;
}

static void
ps_parameters_internal(const stp_vars_t *v, const char *name,
		       stp_parameter_t *description)
{
  int		i;
  ppd_option_t *option;
  ppd_choice_t *choice;
  int status = 0;

  description->p_type = STP_PARAMETER_TYPE_INVALID;
  description->deflt.str = 0;
  description->is_active = 0;

  if (name == NULL)
    return;

  status = check_ppd_file(v);

  for (i = 0; i < the_parameter_count; i++)
  {
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	if (strcmp(name, "PPDFile") == 0)
	  description->is_active = 1;
	else if (strcmp(name, "ModelName") == 0)
	  {
	    if (m_ppd && m_ppd->modelname)
	      {
		description->bounds.str = stp_string_list_create();
		stp_string_list_add_string(description->bounds.str,
					   m_ppd->nickname, m_ppd->nickname);
		description->deflt.str = m_ppd->nickname;
		description->is_active = status;
	      }
	    else
	      description->is_active = 0;
	    return;
	  }
      }
  }

  if (!status)
    return;
  if ((option = stpi_ppdFindOption(m_ppd, name)) == NULL)
  {
    stp_dprintf(STP_DBG_PS, v, "no parameter %s", name);
    return;
  }

  ps_option_to_param(description, NULL, option);
  description->bounds.str = stp_string_list_create();

  stp_dprintf(STP_DBG_PS, v, "describe parameter %s, output name=[%s] text=[%s] category=[%s] choices=[%d]",
	      name, description->name, description->text,
	      description->category, option->num_choices);

  /* Describe all choices for specified option. */
  for (i=0; i < option->num_choices; i++)
  {
    choice = option->choices + i;
    stp_string_list_add_string(description->bounds.str, choice->choice, choice->text);
    if (choice->marked)
      description->deflt.str = choice->choice;
  }

  if (!description->deflt.str)
    description->deflt.str = option->choices[0].choice;
  if (stp_string_list_count(description->bounds.str) > 0)
    description->is_active = 1;
  return;
}

static void
ps_parameters(const stp_vars_t *v, const char *name,
	      stp_parameter_t *description)
{
#ifdef ENABLE_NLS
  char *locale = setlocale(LC_ALL, "C");
#endif
  ps_parameters_internal(v, name, description);
#ifdef ENABLE_NLS
  setlocale(LC_ALL, locale);
#endif
}

/*
 * 'ps_media_size()' - Return the size of the page.
 */

static void
ps_media_size_internal(const stp_vars_t *v,		/* I */
		       int  *width,		/* O - Width in points */
		       int  *height)		/* O - Height in points */
{
  const char *pagesize = stp_get_string_parameter(v, "PageSize");
  int status = check_ppd_file(v);
  if (!pagesize)
    pagesize = "";

  stp_dprintf(STP_DBG_PS, v,
	      "ps_media_size(%d, \'%s\', \'%s\', %p, %p)\n",
	      stp_get_model_id(v), m_ppd_file, pagesize,
	      (void *) width, (void *) height);

  stp_default_media_size(v, width, height);

  if (status)
    {
      *width = stpi_ppdPageWidth(m_ppd, pagesize);
      *height = stpi_ppdPageLength(m_ppd, pagesize);
    }

  stp_dprintf(STP_DBG_PS, v, "dimensions %d %d\n", *width, *height);
  return;
}

static void
ps_media_size(const stp_vars_t *v, int *width, int *height)
{
#ifdef ENABLE_NLS
  char *locale = setlocale(LC_ALL, "C");
#endif
  ps_media_size_internal(v, width, height);
#ifdef ENABLE_NLS
  setlocale(LC_ALL, locale);
#endif
}

/*
 * 'ps_imageable_area()' - Return the imageable area of the page.
 */

static void
ps_imageable_area_internal(const stp_vars_t *v,      /* I */
			   int  use_max_area, /* I - Use maximum area */
			   int  *left,	/* O - Left position in points */
			   int  *right,	/* O - Right position in points */
			   int  *bottom, /* O - Bottom position in points */
			   int  *top)	/* O - Top position in points */
{
  int width, height;
  ppd_size_t *size;
  const char *pagesize = stp_get_string_parameter(v, "PageSize");
  if (!pagesize)
    pagesize = "";

  /* Set some defaults. */
  ps_media_size(v, &width, &height);
  *left   = 0;
  *right  = width;
  *top    = 0;
  *bottom = height;

  if (check_ppd_file(v))
    {
      size = stpi_ppdPageSize(m_ppd, pagesize);
      if (size)
	{
	  stp_dprintf(STP_DBG_PS, v, "size=l %f r %f b %f t %f h %d w %d\n",
		      size->left, size->right, size->top, size->bottom, height, width);
	  *left = (int)size->left;
	  *right = (int)size->right;
	  *top = height - (int)size->top;
	  *bottom = height - (int)size->bottom;
	}
    }

  if (use_max_area)
  {
    if (*left > 0)
      *left = 0;
    if (*right < width)
      *right = width;
    if (*top > 0)
      *top = 0;
    if (*bottom < height)
      *bottom = height;
  }

  stp_dprintf(STP_DBG_PS, v, "pagesize %s max_area=%d l %d r %d b %d t %d h %d w %d\n",
	      pagesize ? pagesize : "(null)",
	      use_max_area, *left, *right, *bottom, *top, width, height);

  return;
}

static void
ps_imageable_area(const stp_vars_t *v,      /* I */
                  int  *left,		/* O - Left position in points */
                  int  *right,		/* O - Right position in points */
                  int  *bottom,		/* O - Bottom position in points */
                  int  *top)		/* O - Top position in points */
{
#ifdef ENABLE_NLS
  char *locale = setlocale(LC_ALL, "C");
#endif
  ps_imageable_area_internal(v, 0, left, right, bottom, top);
#ifdef ENABLE_NLS
  setlocale(LC_ALL, locale);
#endif
}

static void
ps_maximum_imageable_area(const stp_vars_t *v,      /* I */
			  int  *left,	/* O - Left position in points */
			  int  *right,	/* O - Right position in points */
			  int  *bottom,	/* O - Bottom position in points */
			  int  *top)	/* O - Top position in points */
{
#ifdef ENABLE_NLS
  char *locale = setlocale(LC_ALL, "C");
#endif
  ps_imageable_area_internal(v, 1, left, right, bottom, top);
#ifdef ENABLE_NLS
  setlocale(LC_ALL, locale);
#endif
}

static void
ps_limit(const stp_vars_t *v,  		/* I */
	 int *width,
	 int *height,
	 int *min_width,
	 int *min_height)
{
  *width =	INT_MAX;
  *height =	INT_MAX;
  *min_width =	1;
  *min_height =	1;
}

/*
 * This is really bogus...
 */
static void
ps_describe_resolution_internal(const stp_vars_t *v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  *x = -1;
  *y = -1;
  if (resolution)
    sscanf(resolution, "%dx%d", x, y);
  return;
}

static void
ps_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
#ifdef ENABLE_NLS
  char *locale = setlocale(LC_ALL, "C");
#endif
  ps_describe_resolution_internal(v, x, y);
#ifdef ENABLE_NLS
  setlocale(LC_ALL, locale);
#endif
}

static const char *
ps_describe_output(const stp_vars_t *v)
{
  const char *print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *input_image_type = stp_get_string_parameter(v, "InputImageType");
  if (print_mode && strcmp(print_mode, "Color") == 0)
    {
      if (input_image_type && (strcmp(input_image_type, "CMYK") == 0 ||
			       strcmp(input_image_type, "KCMY") == 0))
	return "CMYK";
      else
	return "RGB";
    }
  else
    return "Whitescale";
}

/*
 * 'ps_print()' - Print an image to a PostScript printer.
 */

static int
ps_print_internal(const stp_vars_t *v, stp_image_t *image)
{
  int		status = 1;
  int		model = stp_get_model_id(v);
  const char    *print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *input_image_type = stp_get_string_parameter(v, "InputImageType");
  unsigned short *out = NULL;
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		paper_width,	/* Width of physical page */
		paper_height,	/* Height of physical page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_channels,	/* Output bytes per pixel */
		out_ps_height,	/* Output height (Level 2 output) */
		out_offset;	/* Output offset (Level 2 output) */
  time_t	curtime;	/* Current time of day */
  unsigned	zero_mask;
  int           image_height,
		image_width;
  stp_vars_t	*nv = stp_vars_create_copy(v);
  char		*locale;
  int		color_out = 0;
  int		cmyk_out = 0;

  stp_prune_inactive_options(nv);
  if (!stp_verify(nv))
    {
      stp_eprintf(nv, "Print options not verified; cannot print.\n");
      return 0;
    }
  if (print_mode && strcmp(print_mode, "Color") == 0)
    color_out = 1;
  if (color_out &&
      input_image_type && (strcmp(input_image_type, "CMYK") == 0 ||
			   strcmp(input_image_type, "KCMY") == 0))
    cmyk_out = 1;

  stp_image_init(image);

 /*
  * Compute the output size...
  */

  out_width = stp_get_width(v);
  out_height = stp_get_height(v);

  ps_imageable_area(nv, &page_left, &page_right, &page_bottom, &page_top);
  ps_media_size(v, &paper_width, &paper_height);
  page_width = page_right - page_left;
  page_height = page_bottom - page_top;

  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

 /*
  * Output a standard PostScript header with DSC comments...
  */

  curtime = time(NULL);

  top = paper_height - top;

  stp_dprintf(STP_DBG_PS, v,
	      "out_width = %d, out_height = %d\n", out_width, out_height);
  stp_dprintf(STP_DBG_PS, v,
	      "page_left = %d, page_right = %d, page_bottom = %d, page_top = %d\n",
	      page_left, page_right, page_bottom, page_top);
  stp_dprintf(STP_DBG_PS, v, "left = %d, top = %d\n", left, top);
  stp_dprintf(STP_DBG_PS, v, "page_width = %d, page_height = %d\n",
	      page_width, page_height);

  stp_dprintf(STP_DBG_PS, v, "bounding box l %d b %d r %d t %d\n",
	      page_left, paper_height - page_bottom,
	      page_right, paper_height - page_top);

  stp_puts("%!PS-Adobe-3.0\n", v);
#ifdef HAVE_CONFIG_H
  stp_zprintf(v, "%%%%Creator: %s/Gutenprint %s (%s)\n",
	      stp_image_get_appname(image), VERSION, RELEASE_DATE);
#else
  stp_zprintf(v, "%%%%Creator: %s/Gutenprint\n", stp_image_get_appname(image));
#endif
  stp_zprintf(v, "%%%%CreationDate: %s", ctime(&curtime));
  stp_puts("%Copyright: 1997-2002 by Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu)\n", v);
  stp_zprintf(v, "%%%%BoundingBox: %d %d %d %d\n",
	      page_left, paper_height - page_bottom,
	      page_right, paper_height - page_top);
  stp_puts("%%DocumentData: Clean7Bit\n", v);
  stp_zprintf(v, "%%%%LanguageLevel: %d\n", model + 1);
  stp_puts("%%Pages: 1\n", v);
  stp_puts("%%Orientation: Portrait\n", v);
  stp_puts("%%EndComments\n", v);

#if 0
  /*
   * Removed following device specific commands because device specific commands should be sent separately (ie: via IPP for CUPS). des 7/20/2006
   */

 /*
  * Find any printer-specific commands...
  */

  num_commands = 0;

  if ((command = ppd_find(m_ppd_file, "PageSize", media_size, &order)) != NULL)
  {
    commands[num_commands].keyword = "PageSize";
    commands[num_commands].choice  = media_size;
    commands[num_commands].command = stp_malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(m_ppd_file, "InputSlot", media_source, &order)) != NULL)
  {
    commands[num_commands].keyword = "InputSlot";
    commands[num_commands].choice  = media_source;
    commands[num_commands].command = stp_malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(m_ppd_file, "MediaType", media_type, &order)) != NULL)
  {
    commands[num_commands].keyword = "MediaType";
    commands[num_commands].choice  = media_type;
    commands[num_commands].command = stp_malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(m_ppd_file, "Resolution", resolution, &order)) != NULL)
  {
    commands[num_commands].keyword = "Resolution";
    commands[num_commands].choice  = resolution;
    commands[num_commands].command = stp_malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

 /*
  * Sort the commands using the OrderDependency value...
  */

  for (i = 0; i < (num_commands - 1); i ++)
    for (j = i + 1; j < num_commands; j ++)
      if (commands[j].order < commands[i].order)
      {
        temp                = commands[i].keyword;
        commands[i].keyword = commands[j].keyword;
        commands[j].keyword = temp;

        temp                = commands[i].choice;
        commands[i].choice  = commands[j].choice;
        commands[j].choice  = temp;

        order               = commands[i].order;
        commands[i].order   = commands[j].order;
        commands[j].order   = order;

        command             = commands[i].command;
        commands[i].command = commands[j].command;
        commands[j].command = command;
      }

 /*
  * Send the commands...
  */

  if (num_commands > 0)
  {
    stp_puts("%%BeginSetup\n", v);

    for (i = 0; i < num_commands; i ++)
    {
      stp_puts("[{\n", v);
      stp_zprintf(v, "%%%%BeginFeature: *%s %s\n", commands[i].keyword,
                  commands[i].choice);
      if (commands[i].command[0])
      {
	stp_puts(commands[i].command, v);
	if (commands[i].command[strlen(commands[i].command) - 1] != '\n')
          stp_puts("\n", v);
      }

      stp_puts("%%EndFeature\n", v);
      stp_puts("} stopped cleartomark\n", v);
      stp_free(commands[i].command);
    }

    stp_puts("%%EndSetup\n", v);
  }
#endif

 /*
  * Output the page...
  */

  stp_puts("%%Page: 1 1\n", v);
  stp_puts("gsave\n", v);

  stp_zprintf(v, "%d %d translate\n", left, top);

  /* Force locale to "C", because decimal numbers in Postscript must
     always be printed with a decimal point rather than the
     locale-specific setting. */

#ifdef ENABLE_NLS
  locale = setlocale(LC_ALL, "C");
#endif
  stp_zprintf(v, "%.3f %.3f scale\n",
	      (double)out_width / ((double)image_width),
	      (double)out_height / ((double)image_height));
#ifdef ENABLE_NLS
  setlocale(LC_ALL, locale);
#endif

  stp_channel_reset(nv);
  stp_channel_add(nv, 0, 0, 1.0);
  if (color_out)
    {
      stp_channel_add(nv, 1, 0, 1.0);
      stp_channel_add(nv, 2, 0, 1.0);
      if (cmyk_out)
	{
	  stp_channel_add(nv, 3, 0, 1.0);
	  stp_set_string_parameter(nv, "STPIOutputType", "CMYK");
	}
      else
	stp_set_string_parameter(nv, "STPIOutputType", "RGB");
    }
  else
    stp_set_string_parameter(nv, "STPIOutputType", "Whitescale");

  stp_set_boolean_parameter(nv, "SimpleGamma", 1);

  out_channels = stp_color_init(nv, image, 256);

  if (model == 0)
  {
    stp_zprintf(v, "/picture %d string def\n", image_width * out_channels);

    stp_zprintf(v, "%d %d 8\n", image_width, image_height);

    stp_puts("[ 1 0 0 -1 0 1 ]\n", v);

    if (cmyk_out)
      stp_puts("{currentfile picture readhexstring pop} false 4 colorimage\n", v);
    else if (color_out)
      stp_puts("{currentfile picture readhexstring pop} false 3 colorimage\n", v);
    else
      stp_puts("{currentfile picture readhexstring pop} image\n", v);

    for (y = 0; y < image_height; y ++)
    {
      if (stp_color_get_row(nv, image, y, &zero_mask))
	{
	  status = 2;
	  break;
	}

      out = stp_channel_get_input(nv);

      /* Convert from KCMY to CMYK */
      if (cmyk_out)
	{
	  int x;
	  unsigned short *pos = out;
	  for (x = 0; x < image_width; x++, pos += 4)
	    {
	      unsigned short p0 = pos[0];
	      pos[0] = pos[1];
	      pos[1] = pos[2];
	      pos[2] = pos[3];
	      pos[3] = p0;
	    }
	}
      ps_hex(v, out, image_width * out_channels);
    }
  }
  else
  {
    unsigned short *tmp_buf =
      stp_malloc(sizeof(unsigned short) * (image_width * out_channels + 4));
    if (cmyk_out)
      stp_puts("/DeviceCMYK setcolorspace\n", v);
    else if (color_out)
      stp_puts("/DeviceRGB setcolorspace\n", v);
    else
      stp_puts("/DeviceGray setcolorspace\n", v);

    stp_puts("<<\n", v);
    stp_puts("\t/ImageType 1\n", v);

    stp_zprintf(v, "\t/Width %d\n", image_width);
    stp_zprintf(v, "\t/Height %d\n", image_height);
    stp_puts("\t/BitsPerComponent 8\n", v);

    if (cmyk_out)
      stp_puts("\t/Decode [ 0 1 0 1 0 1 0 1 ]\n", v);
    else if (color_out)
      stp_puts("\t/Decode [ 0 1 0 1 0 1 ]\n", v);
    else
      stp_puts("\t/Decode [ 0 1 ]\n", v);

    stp_puts("\t/DataSource currentfile /ASCII85Decode filter\n", v);

    if ((image_width * 72 / out_width) < 100)
      stp_puts("\t/Interpolate true\n", v);

    stp_puts("\t/ImageMatrix [ 1 0 0 -1 0 1 ]\n", v);

    stp_puts(">>\n", v);
    stp_puts("image\n", v);

    for (y = 0, out_offset = 0; y < image_height; y ++)
    {
      unsigned short *where;
      /* FIXME!!! */
      if (stp_color_get_row(nv, image, y /*, out + out_offset */ , &zero_mask))
	{
	  status = 2;
	  break;
	}
      out = stp_channel_get_input(nv);
      if (out_offset > 0)
	{
	  memcpy(tmp_buf + out_offset, out,
		 image_width * out_channels * sizeof(unsigned short));
	  where = tmp_buf;
	}
      else
	where = out;

      /* Convert from KCMY to CMYK */
      if (cmyk_out)
	{
	  int x;
	  unsigned short *pos = where;
	  for (x = 0; x < image_width; x++, pos += 4)
	    {
	      unsigned short p0 = pos[0];
	      pos[0] = pos[1];
	      pos[1] = pos[2];
	      pos[2] = pos[3];
	      pos[3] = p0;
	    }
	}

      out_ps_height = out_offset + image_width * out_channels;

      if (y < (image_height - 1))
      {
	ps_ascii85(v, where, out_ps_height & ~3, 0);
        out_offset = out_ps_height & 3;
      }
      else
      {
        ps_ascii85(v, where, out_ps_height, 1);
        out_offset = 0;
      }

      if (out_offset > 0)
        memcpy(tmp_buf, where + out_ps_height - out_offset,
	       out_offset * sizeof(unsigned short));
    }
    stp_free(tmp_buf);
  }
  stp_image_conclude(image);

  stp_puts("grestore\n", v);
  stp_puts("showpage\n", v);
  stp_puts("%%Trailer\n", v);
  stp_puts("%%EOF\n", v);
  stp_vars_destroy(nv);
  return status;
}

static int
ps_print(const stp_vars_t *v, stp_image_t *image)
{
#ifdef ENABLE_NLS
  char *locale = setlocale(LC_ALL, "C");
#endif
  int status = ps_print_internal(v, image);
#ifdef ENABLE_NLS
  setlocale(LC_ALL, locale);
#endif
  return status;
}


/*
 * 'ps_hex()' - Print binary data as a series of hexadecimal numbers.
 */

static void
ps_hex(const stp_vars_t *v,	/* I - File to print to */
       unsigned short   *data,	/* I - Data to print */
       int              length)	/* I - Number of bytes to print */
{
  int		col;		/* Current column */
  static const char	*hex = "0123456789ABCDEF";


  col = 0;
  while (length > 0)
  {
    unsigned char pixel = (*data & 0xff00) >> 8;
   /*
    * Put the hex chars out to the file; note that we don't use stp_zprintf()
    * for speed reasons...
    */

    stp_putc(hex[pixel >> 4], v);
    stp_putc(hex[pixel & 15], v);

    data ++;
    length --;

    col += 2;
    if (col >= 72)
    {
      col = 0;
      stp_putc('\n', v);
    }
  }

  if (col > 0)
    stp_putc('\n', v);
}


/*
 * 'ps_ascii85()' - Print binary data as a series of base-85 numbers.
 */

static void
ps_ascii85(const stp_vars_t *v,	/* I - File to print to */
	   unsigned short *data,	/* I - Data to print */
	   int            length,	/* I - Number of bytes to print */
	   int            last_line)	/* I - Last line of raster data? */
{
  int		i;			/* Looping var */
  unsigned	b;			/* Binary data word */
  unsigned char	c[5];			/* ASCII85 encoded chars */
  static int	column = 0;		/* Current column */


  while (length > 3)
  {
    unsigned char d0 = (data[0] & 0xff00) >> 8;
    unsigned char d1 = (data[1] & 0xff00) >> 8;
    unsigned char d2 = (data[2] & 0xff00) >> 8;
    unsigned char d3 = (data[3] & 0xff00) >> 8;
    b = (((((d0 << 8) | d1) << 8) | d2) << 8) | d3;

    if (b == 0)
    {
      stp_putc('z', v);
      column ++;
    }
    else
    {
      c[4] = (b % 85) + '!';
      b /= 85;
      c[3] = (b % 85) + '!';
      b /= 85;
      c[2] = (b % 85) + '!';
      b /= 85;
      c[1] = (b % 85) + '!';
      b /= 85;
      c[0] = b + '!';

      stp_zfwrite((const char *)c, 5, 1, v);
      column += 5;
    }

    if (column > 72)
    {
      stp_putc('\n', v);
      column = 0;
    }

    data += 4;
    length -= 4;
  }

  if (last_line)
  {
    if (length > 0)
    {
      for (b = 0, i = length; i > 0; b = (b << 8) | data[0], data ++, i --);

      c[4] = (b % 85) + '!';
      b /= 85;
      c[3] = (b % 85) + '!';
      b /= 85;
      c[2] = (b % 85) + '!';
      b /= 85;
      c[1] = (b % 85) + '!';
      b /= 85;
      c[0] = b + '!';

      stp_zfwrite((const char *)c, length + 1, 1, v);
    }

    stp_puts("~>\n", v);
    column = 0;
  }
}


static const stp_printfuncs_t print_ps_printfuncs =
{
  ps_list_parameters,
  ps_parameters,
  ps_media_size,
  ps_imageable_area,
  ps_maximum_imageable_area,
  ps_limit,
  ps_print,
  ps_describe_resolution,
  ps_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL
};


static stp_family_t print_ps_module_data =
  {
    &print_ps_printfuncs,
    NULL
  };


static int
print_ps_module_init(void)
{
  return stp_family_register(print_ps_module_data.printer_list);
}


static int
print_ps_module_exit(void)
{
  return stp_family_unregister(print_ps_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_ps_LTX_stp_module_version
#define stp_module_data print_ps_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "ps",
    VERSION,
    "Postscript family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_ps_module_init,
    print_ps_module_exit,
    (void *) &print_ps_module_data
  };

