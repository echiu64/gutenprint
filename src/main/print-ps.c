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
#include <gimp-print/gimp-print.h>
#include <gimp-print/gimp-print-intl-internal.h>
#include "gimp-print-internal.h"
#include "module.h"
#include <time.h>
#include <string.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <stdio.h>

#ifdef _MSC_VER
#define strncasecmp(s,t,n) _strnicmp(s,t,n)
#define strcasecmp(s,t) _stricmp(s,t)
#endif

/*
 * Local variables...
 */

static FILE	*ps_ppd = NULL;
static const char	*ps_ppd_file = NULL;


/*
 * Local functions...
 */

static void	ps_hex(stp_const_vars_t, unsigned short *, int);
static void	ps_ascii85(stp_const_vars_t, unsigned short *, int, int);
static char	*ppd_find(const char *, const char *, const char *, int *);

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_PAGE_SIZE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "MediaType", N_("Media Type"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "InputSlot", N_("Media Source"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "InkType", N_("Ink Type"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "Resolution", N_("Resolution"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "PPDFile", N_("PPDFile"),
    N_("PPD File"),
    STP_PARAMETER_TYPE_FILE, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
};

static int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

/*
 * 'ps_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
ps_list_parameters(stp_const_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  return ret;
}

static void
ps_parameters(stp_const_vars_t v, const char *name,
	      stp_parameter_t *description)
{
  int		i;
  char		line[1024],
		lname[255],
		loption[255],
		*ltext;
  const char *ppd_file = stp_get_file_parameter(v, "PPDFile");
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  description->deflt.str = 0;

  if (name == NULL)
    return;

  if (ppd_file != NULL && strlen(ppd_file) > 0 &&
      (ps_ppd_file == NULL || strcmp(ps_ppd_file, ppd_file) != 0))
  {
    if (ps_ppd != NULL)
      fclose(ps_ppd);

    ps_ppd = fopen(ppd_file, "r");

    if (ps_ppd == NULL)
      ps_ppd_file = NULL;
    else
      ps_ppd_file = ppd_file;
  }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stpi_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }

  if (ps_ppd == NULL)
    {
      if (strcmp(name, "PageSize") == 0)
	{
	  int papersizes = stp_known_papersizes();
	  description->bounds.str = stp_string_list_create();
	  for (i = 0; i < papersizes; i++)
	    {
	      const stp_papersize_t *pt = stp_get_papersize_by_index(i);
	      if (strlen(pt->name) > 0)
		stp_string_list_add_string
		  (description->bounds.str, pt->name, pt->text);
	    }
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	  description->is_active = 1;
	}
      else
	description->is_active = 0;
      return;
    }

  rewind(ps_ppd);
  description->bounds.str = stp_string_list_create();

  while (fgets(line, sizeof(line), ps_ppd) != NULL)
  {
    if (line[0] != '*')
      continue;

    if (sscanf(line, "*%s %[^:]", lname, loption) != 2)
      continue;

    if (strcasecmp(lname, name) == 0)
    {
      if ((ltext = strchr(loption, '/')) != NULL)
        *ltext++ = '\0';
      else
        ltext = loption;

      stp_string_list_add_string(description->bounds.str, loption, ltext);
    }
  }

  if (stp_string_list_count(description->bounds.str) > 0)
    description->deflt.str =
      stp_string_list_param(description->bounds.str, 0)->name;
  else
    description->is_active = 0;
  return;
}

/*
 * 'ps_media_size()' - Return the size of the page.
 */

static void
ps_media_size(stp_const_vars_t v,		/* I */
              int  *width,		/* O - Width in points */
              int  *height)		/* O - Height in points */
{
  char	*dimensions;			/* Dimensions of media size */
  const char *pagesize = stp_get_string_parameter(v, "PageSize");
  const char *ppd_file_name = stp_get_file_parameter(v, "PPDFile");
  if (!pagesize)
    pagesize = "";

  stpi_dprintf(STPI_DBG_PS, v,
	      "ps_media_size(%d, \'%s\', \'%s\', %p, %p)\n",
	      stpi_get_model_id(v), ppd_file_name, pagesize,
	       (void *) width, (void *) height);

  if ((dimensions = ppd_find(ppd_file_name, "PaperDimension", pagesize, NULL))
      != NULL)
    sscanf(dimensions, "%d%d", width, height);
  else
    stpi_default_media_size(v, width, height);
}


/*
 * 'ps_imageable_area()' - Return the imageable area of the page.
 */

static void
ps_imageable_area(stp_const_vars_t v,      /* I */
                  int  *left,		/* O - Left position in points */
                  int  *right,		/* O - Right position in points */
                  int  *bottom,		/* O - Bottom position in points */
                  int  *top)		/* O - Top position in points */
{
  char	*area;				/* Imageable area of media */
  float	fleft,				/* Floating point versions */
	fright,
	fbottom,
	ftop;
  int width, height;
  const char *pagesize = stp_get_string_parameter(v, "PageSize");
  if (!pagesize)
    pagesize = "";
  ps_media_size(v, &width, &height);

  if ((area = ppd_find(stp_get_file_parameter(v, "PPDFile"),
		       "ImageableArea", pagesize, NULL))
      != NULL)
    {
      stpi_dprintf(STPI_DBG_PS, v, "area = \'%s\'\n", area);
      if (sscanf(area, "%f%f%f%f", &fleft, &fbottom, &fright, &ftop) == 4)
	{
	  *left   = (int)fleft;
	  *right  = (int)fright;
	  *bottom = height - (int)fbottom;
	  *top    = height - (int)ftop;
	}
      else
	*left = *right = *bottom = *top = 0;
    }
  else
    {
      *left   = 18;
      *right  = width - 18;
      *top    = 36;
      *bottom = height - 36;
    }
}

static void
ps_limit(stp_const_vars_t v,  		/* I */
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
ps_describe_resolution(stp_const_vars_t v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  *x = -1;
  *y = -1;
  if (resolution)
    sscanf(resolution, "%dx%d", x, y);
  return;
}

/*
 * 'ps_print()' - Print an image to a PostScript printer.
 */

static int
ps_print(stp_const_vars_t v, stp_image_t *image)
{
  int		status = 1;
  int		model = stpi_get_model_id(v);
  const char	*ppd_file = stp_get_file_parameter(v, "PPDFile");
  const char	*resolution = stp_get_string_parameter(v, "Resolution");
  const char	*media_size = stp_get_string_parameter(v, "PageSize");
  const char	*media_type = stp_get_string_parameter(v, "MediaType");
  const char	*media_source = stp_get_string_parameter(v, "InputSlot");
  int 		output_type = stp_get_output_type(v);
  unsigned short *out = NULL;
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		i, j;		/* Looping vars */
  int		y;		/* Looping vars */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_channels,	/* Output bytes per pixel */
		out_ps_height,	/* Output height (Level 2 output) */
		out_offset;	/* Output offset (Level 2 output) */
  time_t	curtime;	/* Current time of day */
  unsigned	zero_mask;
  char		*command;	/* PostScript command */
  const char	*temp;		/* Temporary string pointer */
  int		order,		/* Order of command */
		num_commands;	/* Number of commands */
  struct			/* PostScript commands... */
  {
    const char	*keyword, *choice;
    char	*command;
    int		order;
  }		commands[4];
  int           image_height,
                image_width,
                image_bpp;
  stp_vars_t	nv = stp_vars_create_copy(v);
  if (!resolution)
    resolution = "";
  if (!media_size)
    media_size = "";
  if (!media_type)
    media_type = "";
  if (!media_source)
    media_source = "";

  stpi_prune_inactive_options(nv);
  if (!stp_verify(nv))
    {
      stpi_eprintf(nv, "Print options not verified; cannot print.\n");
      return 0;
    }

 /*
  * Setup a read-only pixel region for the entire image...
  */

  stpi_image_init(image);
  image_bpp = stpi_image_bpp(image);

 /*
  * Compute the output size...
  */

  out_width = stp_get_width(v);
  out_height = stp_get_height(v);

  ps_imageable_area(nv, &page_left, &page_right, &page_bottom, &page_top);
  left -= page_left;
  top -= page_top;
  page_width = page_right - page_left;
  page_height = page_bottom - page_top;

  image_height = stpi_image_height(image);
  image_width = stpi_image_width(image);

 /*
  * Let the user know what we're doing...
  */

  stpi_image_progress_init(image);

 /*
  * Output a standard PostScript header with DSC comments...
  */

  curtime = time(NULL);

  left += page_left;

  top = page_height - top + page_bottom;

  stpi_dprintf(STPI_DBG_PS, v,
	      "out_width = %d, out_height = %d\n", out_width, out_height);
  stpi_dprintf(STPI_DBG_PS, v,
	      "page_left = %d, page_right = %d, page_bottom = %d, page_top = %d\n",
	      page_left, page_right, page_bottom, page_top);
  stpi_dprintf(STPI_DBG_PS, v, "left = %d, top = %d\n", left, top);

  stpi_puts("%!PS-Adobe-3.0\n", v);
#ifdef HAVE_CONFIG_H
  stpi_zprintf(v, "%%%%Creator: %s/Gimp-Print %s (%s)\n",
	      stpi_image_get_appname(image), VERSION, RELEASE_DATE);
#else
  stpi_zprintf(v, "%%%%Creator: %s/Gimp-Print\n", stpi_image_get_appname(image));
#endif
  stpi_zprintf(v, "%%%%CreationDate: %s", ctime(&curtime));
  stpi_puts("%Copyright: 1997-2002 by Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu)\n", v);
  stpi_zprintf(v, "%%%%BoundingBox: %d %d %d %d\n",
          left, top - out_height, left + out_width, top);
  stpi_puts("%%DocumentData: Clean7Bit\n", v);
  stpi_zprintf(v, "%%%%LanguageLevel: %d\n", model + 1);
  stpi_puts("%%Pages: 1\n", v);
  stpi_puts("%%Orientation: Portrait\n", v);
  stpi_puts("%%EndComments\n", v);

 /*
  * Find any printer-specific commands...
  */

  num_commands = 0;

  if ((command = ppd_find(ppd_file, "PageSize", media_size, &order)) != NULL)
  {
    commands[num_commands].keyword = "PageSize";
    commands[num_commands].choice  = media_size;
    commands[num_commands].command = stpi_malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(ppd_file, "InputSlot", media_source, &order)) != NULL)
  {
    commands[num_commands].keyword = "InputSlot";
    commands[num_commands].choice  = media_source;
    commands[num_commands].command = stpi_malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(ppd_file, "MediaType", media_type, &order)) != NULL)
  {
    commands[num_commands].keyword = "MediaType";
    commands[num_commands].choice  = media_type;
    commands[num_commands].command = stpi_malloc(strlen(command) + 1);
    strcpy(commands[num_commands].command, command);
    commands[num_commands].order   = order;
    num_commands ++;
  }

  if ((command = ppd_find(ppd_file, "Resolution", resolution, &order)) != NULL)
  {
    commands[num_commands].keyword = "Resolution";
    commands[num_commands].choice  = resolution;
    commands[num_commands].command = stpi_malloc(strlen(command) + 1);
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
    stpi_puts("%%BeginSetup\n", v);

    for (i = 0; i < num_commands; i ++)
    {
      stpi_puts("[{\n", v);
      stpi_zprintf(v, "%%%%BeginFeature: *%s %s\n", commands[i].keyword,
                  commands[i].choice);
      if (commands[i].command[0])
      {
	stpi_puts(commands[i].command, v);
	if (commands[i].command[strlen(commands[i].command) - 1] != '\n')
          stpi_puts("\n", v);
      }

      stpi_puts("%%EndFeature\n", v);
      stpi_puts("} stopped cleartomark\n", v);
      stpi_free(commands[i].command);
    }

    stpi_puts("%%EndSetup\n", v);
  }

 /*
  * Output the page...
  */

  stpi_puts("%%Page: 1 1\n", v);
  stpi_puts("gsave\n", v);

  stpi_zprintf(v, "%d %d translate\n", left, top);

  /* Force locale to "C", because decimal numbers in Postscript must
     always be printed with a decimal point rather than the
     locale-specific setting. */

  setlocale(LC_ALL, "C");
  stpi_zprintf(v, "%.3f %.3f scale\n",
          (double)out_width / ((double)image_width),
          (double)out_height / ((double)image_height));
  setlocale(LC_ALL, "");

  stpi_channel_reset(nv);
  stpi_channel_add(nv, 0, 0, 1.0);
  if (output_type == OUTPUT_COLOR)
    {
      stpi_channel_add(nv, 1, 0, 1.0);
      stpi_channel_add(nv, 2, 0, 1.0);
    }

  out_channels = stpi_color_init(nv, image, 256);

  if (model == 0)
  {
    stpi_zprintf(v, "/picture %d string def\n", image_width * out_channels);

    stpi_zprintf(v, "%d %d 8\n", image_width, image_height);

    stpi_puts("[ 1 0 0 -1 0 1 ]\n", v);

    if (output_type == OUTPUT_GRAY)
      stpi_puts("{currentfile picture readhexstring pop} image\n", v);
    else
      stpi_puts("{currentfile picture readhexstring pop} false 3 colorimage\n", v);

    for (y = 0; y < image_height; y ++)
    {
      if ((y & 15) == 0)
	stpi_image_note_progress(image, y, image_height);

      if (stpi_color_get_row(nv, image, y, &zero_mask))
	{
	  status = 2;
	  break;
	}

      out = stpi_channel_get_input(nv);
      ps_hex(v, out, image_width * out_channels);
    }
  }
  else
  {
    if (output_type == OUTPUT_GRAY)
      stpi_puts("/DeviceGray setcolorspace\n", v);
    else
      stpi_puts("/DeviceRGB setcolorspace\n", v);

    stpi_puts("<<\n", v);
    stpi_puts("\t/ImageType 1\n", v);

    stpi_zprintf(v, "\t/Width %d\n", image_width);
    stpi_zprintf(v, "\t/Height %d\n", image_height);
    stpi_puts("\t/BitsPerComponent 8\n", v);

    if (output_type == OUTPUT_GRAY)
      stpi_puts("\t/Decode [ 0 1 ]\n", v);
    else
      stpi_puts("\t/Decode [ 0 1 0 1 0 1 ]\n", v);

    stpi_puts("\t/DataSource currentfile /ASCII85Decode filter\n", v);

    if ((image_width * 72 / out_width) < 100)
      stpi_puts("\t/Interpolate true\n", v);

    stpi_puts("\t/ImageMatrix [ 1 0 0 -1 0 1 ]\n", v);

    stpi_puts(">>\n", v);
    stpi_puts("image\n", v);

    for (y = 0, out_offset = 0; y < image_height; y ++)
    {
      if ((y & 15) == 0)
	stpi_image_note_progress(image, y, image_height);

      /* FIXME!!! */
      if (stpi_color_get_row(nv, image, y /*, out + out_offset */ , &zero_mask))
	{
	  status = 2;
	  break;
	}
      out = stpi_channel_get_input(nv);

      out_ps_height = out_offset + image_width * out_channels;

      if (y < (image_height - 1))
      {
        ps_ascii85(v, out, out_ps_height & ~3, 0);
        out_offset = out_ps_height & 3;
      }
      else
      {
        ps_ascii85(v, out, out_ps_height, 1);
        out_offset = 0;
      }

      if (out_offset > 0)
        memcpy(out, out + out_ps_height - out_offset, out_offset);
    }
  }
  stpi_image_progress_conclude(image);

  if (out)
    stpi_free(out);

  stpi_puts("grestore\n", v);
  stpi_puts("showpage\n", v);
  stpi_puts("%%Trailer\n", v);
  stpi_puts("%%EOF\n", v);
  stp_vars_free(nv);
  return status;
}


/*
 * 'ps_hex()' - Print binary data as a series of hexadecimal numbers.
 */

static void
ps_hex(stp_const_vars_t v,	/* I - File to print to */
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
    * Put the hex chars out to the file; note that we don't use stpi_zprintf()
    * for speed reasons...
    */

    stpi_putc(hex[pixel >> 4], v);
    stpi_putc(hex[pixel & 15], v);

    data ++;
    length --;

    col += 2;
    if (col >= 72)
    {
      col = 0;
      stpi_putc('\n', v);
    }
  }

  if (col > 0)
    stpi_putc('\n', v);
}


/*
 * 'ps_ascii85()' - Print binary data as a series of base-85 numbers.
 */

static void
ps_ascii85(stp_const_vars_t v,	/* I - File to print to */
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
      stpi_putc('z', v);
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

      stpi_zfwrite((const char *)c, 5, 1, v);
      column += 5;
    }

    if (column > 72)
    {
      stpi_putc('\n', v);
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

      stpi_zfwrite((const char *)c, length + 1, 1, v);
    }

    stpi_puts("~>\n", v);
    column = 0;
  }
}


/*
 * 'ppd_find()' - Find a control string with the specified name & parameters.
 */

static char *			/* O - Control string */
ppd_find(const char *ppd_file,	/* I - Name of PPD file */
         const char *name,	/* I - Name of parameter */
         const char *option,	/* I - Value of parameter */
         int  *order)		/* O - Order of the control string */
{
  char		line[1024],	/* Line from file */
		lname[255],	/* Name from line */
		loption[255],	/* Value from line */
		*opt;		/* Current control string pointer */
  static char	*value = NULL;	/* Current control string value */


  if (ppd_file == NULL || name == NULL || option == NULL)
    return (NULL);
  if (!value)
    value = stpi_zalloc(32768);

  if (ps_ppd_file == NULL || strcmp(ps_ppd_file, ppd_file) != 0)
  {
    if (ps_ppd != NULL)
      fclose(ps_ppd);

    ps_ppd = fopen(ppd_file, "r");

    if (ps_ppd == NULL)
      ps_ppd_file = NULL;
    else
      ps_ppd_file = ppd_file;
  }

  if (ps_ppd == NULL)
    return (NULL);

  if (order != NULL)
    *order = 1000;

  rewind(ps_ppd);
  while (fgets(line, sizeof(line), ps_ppd) != NULL)
  {
    if (line[0] != '*')
      continue;

    if (strncasecmp(line, "*OrderDependency:", 17) == 0 && order != NULL)
    {
      sscanf(line, "%*s%d", order);
      continue;
    }
    else if (sscanf(line, "*%s %[^/:]", lname, loption) != 2)
      continue;

    if (strcasecmp(lname, name) == 0 &&
        strcasecmp(loption, option) == 0)
    {
      opt = strchr(line, ':') + 1;
      while (*opt == ' ' || *opt == '\t')
        opt ++;
      if (*opt != '\"')
        continue;

      strcpy(value, opt + 1);
      if ((opt = strchr(value, '\"')) == NULL)
      {
        while (fgets(line, sizeof(line), ps_ppd) != NULL)
        {
          strcat(value, line);
          if (strchr(line, '\"') != NULL)
          {
            strcpy(strchr(value, '\"'), "\n");
            break;
          }
        }
      }
      else
        *opt = '\0';

      return (value);
    }
  }

  return (NULL);
}

static const stpi_printfuncs_t print_ps_printfuncs =
{
  ps_list_parameters,
  ps_parameters,
  ps_media_size,
  ps_imageable_area,
  ps_limit,
  ps_print,
  ps_describe_resolution,
  stpi_verify_printer_params,
  NULL,
  NULL
};


static stpi_internal_family_t print_ps_module_data =
  {
    &print_ps_printfuncs,
    NULL
  };


static int
print_ps_module_init(void)
{
  return stpi_family_register(print_ps_module_data.printer_list);
}


static int
print_ps_module_exit(void)
{
  return stpi_family_unregister(print_ps_module_data.printer_list);
}


/* Module header */
#define stpi_module_version print_ps_LTX_stpi_module_version
#define stpi_module_data print_ps_LTX_stpi_module_data

stpi_module_version_t stpi_module_version = {0, 0};

stpi_module_t stpi_module_data =
  {
    "ps",
    VERSION,
    "Postscript family driver",
    STPI_MODULE_CLASS_FAMILY,
    NULL,
    print_ps_module_init,
    print_ps_module_exit,
    (void *) &print_ps_module_data
  };

