/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
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
#include "../../lib/libprintut.h"

#include <gimp-print/gimp-print-intl-internal.h>
#include <gimp-print-ui/gimp-print-ui.h>
#include "gimp-print-ui-internal.h"

#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>


static int	compare_printers (stpui_plist_t *p1, stpui_plist_t *p2);

int		stpui_plist_current = 0,	/* Current system printer */
		stpui_plist_count = 0;	/* Number of system printers */
stpui_plist_t	*stpui_plist;			/* System printers */
int             stpui_show_all_paper_sizes = 0;
static char *printrc_name = NULL;
static char *image_type;
static gint image_raw_channels = 0;
static gint image_channel_depth = 8;

#define STP_SAFE_FREE(x)			\
do						\
{						\
  if ((x))					\
    free((char *)(x));				\
  ((x)) = NULL;					\
} while (0)

void
stpui_set_printrc_file(const char *name)
{
  if (name && name == printrc_name)
    return;
  STP_SAFE_FREE(printrc_name);
  if (name)
    printrc_name = g_strdup(name);
  else
    {
      const char *where = getenv("HOME");
      if (where)
	printrc_name = g_strconcat(where, "/.gimpprintrc");
    }
}

const char *
stpui_get_printrc_file(void)
{
  if (!printrc_name)
    stpui_set_printrc_file(NULL);
  return printrc_name;
}

void
stpui_plist_set_output_to(stpui_plist_t *p, const char *val)
{
  if (p->output_to == val)
    return;
  STP_SAFE_FREE(p->output_to);
  p->output_to = g_strdup(val);
}

void
stpui_plist_set_output_to_n(stpui_plist_t *p, const char *val, int n)
{
  if (p->output_to == val)
    return;
  STP_SAFE_FREE(p->output_to);
  p->output_to = g_strndup(val, n);
}

const char *
stpui_plist_get_output_to(const stpui_plist_t *p)
{
  return p->output_to;
}

void
stpui_plist_set_name(stpui_plist_t *p, const char *val)
{
  if (p->name == val)
    return;
  STP_SAFE_FREE(p->name);
  p->name = g_strdup(val);
}

void
stpui_plist_set_name_n(stpui_plist_t *p, const char *val, int n)
{
  if (p->name == val)
    return;
  STP_SAFE_FREE(p->name);
  p->name = g_strndup(val, n);
}

const char *
stpui_plist_get_name(const stpui_plist_t *p)
{
  return p->name;
}

void
stpui_set_image_type(const char *itype)
{
  image_type = g_strdup(itype);
}

void
stpui_set_image_raw_channels(gint channels)
{
  image_raw_channels = channels;
}

void
stpui_set_image_channel_depth(gint depth)
{
  image_channel_depth = depth;
}

void
stpui_printer_initialize(stpui_plist_t *printer)
{
  char tmp[32];
  stpui_plist_set_output_to(printer, "");
  stpui_plist_set_name(printer, "");
  printer->active = 0;
  printer->scaling = 100.0;
  printer->orientation = ORIENT_AUTO;
  printer->auto_size_roll_feed_paper = 0;
  printer->unit = 0;
  printer->v = stp_vars_create();
  stp_set_string_parameter(printer->v, "InputImageType", image_type);
  if (image_raw_channels)
    {
      (void) sprintf(tmp, "%d", image_raw_channels);
      stp_set_string_parameter(printer->v, "RawChannels", tmp);
    }
  if (image_channel_depth)
    {
      (void) sprintf(tmp, "%d", image_channel_depth);
      stp_set_string_parameter(printer->v, "ChannelBitDepth", tmp);
    }
  printer->invalid_mask = INVALID_TOP | INVALID_LEFT;
}

static void
stpui_plist_destroy(stpui_plist_t *printer)
{
  STP_SAFE_FREE(printer->name);
  STP_SAFE_FREE(printer->output_to);
  stp_vars_destroy(printer->v);
}

void
stpui_plist_copy(stpui_plist_t *vd, const stpui_plist_t *vs)
{
  if (vs == vd)
    return;
  stp_vars_copy(vd->v, vs->v);
/*  vd->active = vs->active; */
  vd->scaling = vs->scaling;
  vd->orientation = vs->orientation;
  vd->auto_size_roll_feed_paper = vs->auto_size_roll_feed_paper;
  vd->unit = vs->unit;
  vd->invalid_mask = vs->invalid_mask;
  stpui_plist_set_name(vd, stpui_plist_get_name(vs));
  stpui_plist_set_output_to(vd, stpui_plist_get_output_to(vs));
}

static stpui_plist_t *
allocate_stpui_plist_copy(const stpui_plist_t *vs)
{
  stpui_plist_t *rep = xmalloc(sizeof(stpui_plist_t));
  memset(rep, 0, sizeof(stpui_plist_t));
  rep->v = stp_vars_create();
  stpui_plist_copy(rep, vs);
  return rep;
}

static void
check_plist(int count)
{
  static int current_plist_size = 0;
  int i;
  if (count <= current_plist_size)
    return;
  else if (current_plist_size == 0)
    {
      current_plist_size = count;
      stpui_plist = xmalloc(current_plist_size * sizeof(stpui_plist_t));
      for (i = 0; i < current_plist_size; i++)
	{
	  memset(&(stpui_plist[i]), 0, sizeof(stpui_plist_t));
	  stpui_printer_initialize(&(stpui_plist[i]));
	}
    }
  else
    {
      int old_plist_size = current_plist_size;
      current_plist_size *= 2;
      if (current_plist_size < count)
	current_plist_size = count;
      stpui_plist = realloc(stpui_plist, current_plist_size * sizeof(stpui_plist_t));
      for (i = old_plist_size; i < current_plist_size; i++)
	{
	  memset(&(stpui_plist[i]), 0, sizeof(stpui_plist_t));
	  stpui_printer_initialize(&(stpui_plist[i]));
	}
    }
}

#define GET_MANDATORY_INTERNAL_STRING_PARAM(param)		\
do {								\
  if ((commaptr = strchr(lineptr, ',')) == NULL)		\
    continue;							\
  stpui_plist_set_##param##_n(&key, lineptr, commaptr - line);	\
  lineptr = commaptr + 1;					\
} while (0)

#define GET_MANDATORY_STRING_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param##_n(key.v, lineptr, commaptr - line);	\
  lineptr = commaptr + 1;				\
} while (0)

static int
get_mandatory_string_param(stp_vars_t *v, const char *param, char **lineptr)
{
  char *commaptr = strchr(*lineptr, ',');
  if (commaptr == NULL)
    return 0;
  stp_set_string_parameter_n(v, param, *lineptr, commaptr - *lineptr);
  *lineptr = commaptr + 1;
  return 1;
}

static int
get_mandatory_file_param(stp_vars_t *v, const char *param, char **lineptr)
{
  char *commaptr = strchr(*lineptr, ',');
  if (commaptr == NULL)
    return 0;
  stp_set_file_parameter_n(v, param, *lineptr, commaptr - *lineptr);
  *lineptr = commaptr + 1;
  return 1;
}

#define GET_MANDATORY_INT_PARAM(param)			\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param(key.v, atoi(lineptr));		\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_MANDATORY_INTERNAL_INT_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  key.param = atoi(lineptr);				\
  lineptr = commaptr + 1;				\
} while (0)

static void
get_optional_string_param(stp_vars_t *v, const char *param,
			  char **lineptr, int *keepgoing)
{
  if (*keepgoing)
    {
      char *commaptr = strchr(*lineptr, ',');
      if (commaptr == NULL)
	{
	  stp_set_string_parameter(v, param, *lineptr);
	  *keepgoing = 0;
	}
      else
	{
	  stp_set_string_parameter_n(v, param, *lineptr, commaptr - *lineptr);
	  *lineptr = commaptr + 1;
	}
    }
}

#define GET_OPTIONAL_INT_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      stp_set_##param(key.v, atoi(lineptr));				\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define GET_OPTIONAL_INTERNAL_INT_PARAM(param)				\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.param = atoi(lineptr);					\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define IGNORE_OPTIONAL_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

static void
get_optional_float_param(stp_vars_t *v, const char *param,
			 char **lineptr, int *keepgoing)
{
  if (*keepgoing)
    {
      char *commaptr = strchr(*lineptr, ',');
      if (commaptr == NULL)
	{
	  stp_set_float_parameter(v, param, atof(*lineptr));
	  *keepgoing = 0;
	}
      else
	{
	  stp_set_float_parameter(v, param, atof(*lineptr));
	  *lineptr = commaptr + 1;
	}
    }
}

#define GET_OPTIONAL_INTERNAL_FLOAT_PARAM(param)			\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.param = atof(lineptr);					\
    }									\
} while (0)

static void *
psearch(const void *key, const void *base, size_t nmemb, size_t size,
	int (*compar)(const void *, const void *))
{
  int i;
  const char *cbase = (const char *) base;
  for (i = 0; i < nmemb; i++)
    {
      if ((*compar)(key, (const void *) cbase) == 0)
	return (void *) cbase;
      cbase += size;
    }
  return NULL;
}

stpui_plist_t *
stpui_plist_create(const char *name, const char *driver)
{
  stpui_plist_t key;
  stpui_plist_t *answer = NULL;
  memset(&key, 0, sizeof(key));
  stpui_printer_initialize(&key);
  key.invalid_mask = 0;
  if (strcmp(name, "File") == 0)
    stpui_plist_set_name(&key, _("File"));
  else
    stpui_plist_set_name(&key, name);
  stp_set_driver(key.v, driver);
  if (stpui_plist_add(&key, 0))
    answer = psearch(&key, stpui_plist, stpui_plist_count,
		     sizeof(stpui_plist_t),
		     (int (*)(const void *, const void *)) compare_printers);
  free(key.name);
  free(key.output_to);
  stp_vars_destroy(key.v);
  return answer;
}

int
stpui_plist_add(const stpui_plist_t *key, int add_only)
{
  /*
   * The format of the list is the File printer followed by a qsort'ed list
   * of system printers. So, if we want to update the file printer, it is
   * always first in the list, else call psearch.
   */
  stpui_plist_t *p;
  if (strcmp(_("File"), key->name) == 0 &&
      strcmp(stpui_plist[0].name, _("File")) == 0)
    {
      if (add_only)
	return 0;
      if (stp_get_printer(key->v))
	{
#ifdef DEBUG
	  fprintf(stderr, "Updated File printer directly\n");
#endif
	  p = &stpui_plist[0];
	  stpui_plist_copy(p, key);
	  p->active = 1;
	}
      return 1;
    }
  else if (stp_get_printer(key->v))
    {
      p = psearch(key, stpui_plist + 1, stpui_plist_count - 1,
		  sizeof(stpui_plist_t),
		  (int (*)(const void *, const void *)) compare_printers);
      if (p == NULL)
	{
#ifdef DEBUG
	  fprintf(stderr, "Adding new printer from printrc file: %s\n",
		  key->name);
#endif
	  check_plist(stpui_plist_count + 1);
	  p = stpui_plist + stpui_plist_count;
	  stpui_plist_count++;
	  stpui_plist_copy(p, key);
	}
      else
	{
	  if (add_only)
	    return 0;
#ifdef DEBUG
	  fprintf(stderr, "Updating printer %s.\n", key->name);
#endif
	  stpui_plist_copy(p, key);
	}
    }
  return 1;
}

static void
stpui_printrc_load_v0(FILE *fp)
{
  char		line[1024];	/* Line in printrc file */
  char		*lineptr;	/* Pointer in line */
  char		*commaptr;	/* Pointer to next comma */
  stpui_plist_t	key;		/* Search key */
  int keepgoing = 1;
  (void) memset(line, 0, 1024);
  (void) memset(&key, 0, sizeof(stpui_plist_t));
  stpui_printer_initialize(&key);
  key.name = g_strdup(_("File"));
  key.active = 1;
  while (fgets(line, sizeof(line), fp) != NULL)
    {
      /*
       * Read old format printrc lines...
       */

      stpui_printer_initialize(&key);
      key.invalid_mask = 0;
      lineptr = line;

      /*
       * Read the command-delimited printer definition data.  Note that
       * we can't use sscanf because %[^,] fails if the string is empty...
       */

      GET_MANDATORY_INTERNAL_STRING_PARAM(name);
      GET_MANDATORY_INTERNAL_STRING_PARAM(output_to);
      GET_MANDATORY_STRING_PARAM(driver);

      if (! stp_get_printer(key.v))
	continue;

      if (!get_mandatory_file_param(key.v, "PPDFile", &lineptr))
	continue;
      if ((commaptr = strchr(lineptr, ',')) != NULL)
	{
	  switch (atoi(lineptr))
	    {
	    case 1:
	      stp_set_string_parameter(key.v, "PrintingMode", "Color");
	      break;
	    case 0:
	    default:
	      stp_set_string_parameter(key.v, "PrintingMode", "BW");
	      break;
	    }
	}
      else
	continue;
      
      if (!get_mandatory_string_param(key.v, "Resolution", &lineptr))
	continue;
      if (!get_mandatory_string_param(key.v, "PageSize", &lineptr))
	continue;
      if (!get_mandatory_string_param(key.v, "MediaType", &lineptr))
	continue;

      get_optional_string_param(key.v, "InputSlot", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Brightness", &lineptr, &keepgoing);

      GET_OPTIONAL_INTERNAL_FLOAT_PARAM(scaling);
      GET_OPTIONAL_INTERNAL_INT_PARAM(orientation);
      GET_OPTIONAL_INT_PARAM(left);
      GET_OPTIONAL_INT_PARAM(top);
      get_optional_float_param(key.v, "Gamma", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Contrast", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Cyan", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Magenta", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Yellow", &lineptr, &keepgoing);
      IGNORE_OPTIONAL_PARAM(linear);
      IGNORE_OPTIONAL_PARAM(image_type);
      get_optional_float_param(key.v, "Saturation", &lineptr, &keepgoing);
      get_optional_float_param(key.v, "Density", &lineptr, &keepgoing);
      get_optional_string_param(key.v, "InkType", &lineptr, &keepgoing);
      get_optional_string_param(key.v,"DitherAlgorithm",&lineptr,&keepgoing);
      GET_OPTIONAL_INTERNAL_INT_PARAM(unit);
      stpui_plist_add(&key, 0);
      free(key.name);
      free(key.output_to);
      stp_vars_destroy(key.v);
    }
  stpui_plist_current = 0;
}

static void
stpui_printrc_load_v1(FILE *fp)
{
  char		line[1024];	/* Line in printrc file */
  stpui_plist_t	key;		/* Search key */
  char *	current_printer = 0; /* printer to select */
  (void) memset(line, 0, 1024);
  (void) memset(&key, 0, sizeof(stpui_plist_t));
  stpui_printer_initialize(&key);
  key.name = g_strdup(_("File"));
  key.active = 1;
  while (fgets(line, sizeof(line), fp) != NULL)
    {
      /*
       * Read new format printrc lines...
       */

      char *keyword, *end, *value;

      keyword = line;
      for (keyword = line; isspace(*keyword); keyword++)
	{
	  /* skip initial spaces... */
	}
      if (!isalpha(*keyword))
	continue;
      for (end = keyword; isalnum(*end) || *end == '-'; end++)
	{
	  /* find end of keyword... */
	}
      value = end;
      while (isspace(*value))
	{
	  /* skip over white space... */
	  value++;
	}
      if (*value != ':')
	continue;
      value++;
      *end = '\0';
      while (isspace(*value))
	{
	  /* skip over white space... */
	  value++;
	}
      for (end = value; *end && *end != '\n'; end++)
	{
	  /* find end of line... */
	}
      *end = '\0';
#ifdef DEBUG
      fprintf(stderr, "Keyword = `%s', value = `%s'\n", keyword, value);
#endif
      if (strcasecmp("current-printer", keyword) == 0)
	{
	  if (current_printer)
	    free (current_printer);
	  current_printer = g_strdup(value);
	}
      else if (strcasecmp("printer", keyword) == 0)
	{
	  /* Switch to printer named VALUE */
	  stpui_plist_add(&key, 0);
#ifdef DEBUG
	  fprintf(stderr,
		  "output_to is now %s\n", stpui_plist_get_output_to(&key));
#endif

	  stp_vars_destroy(key.v);
	  stpui_printer_initialize(&key);
	  key.invalid_mask = 0;
	  stpui_plist_set_name(&key, value);
	}
      else if (strcasecmp("destination", keyword) == 0)
	stpui_plist_set_output_to(&key, value);
      else if (strcasecmp("driver", keyword) == 0)
	stp_set_driver(key.v, value);
      else if (strcasecmp("ppd-file", keyword) == 0)
	stp_set_file_parameter(key.v, "PPDFile", value);
      else if (strcasecmp("output-type", keyword) == 0)
	{
	  switch (atoi(value))
	    {
	    case 1:
	      stp_set_string_parameter(key.v, "PrintingMode", "Color");
	      break;
	    case 0:
	    default:
	      stp_set_string_parameter(key.v, "PrintingMode", "BW");
	      break;
	    }
	}
      else if (strcasecmp("media-size", keyword) == 0)
	stp_set_string_parameter(key.v, "PageSize", value);
      else if (strcasecmp("media-type", keyword) == 0)
	stp_set_string_parameter(key.v, "MediaType", value);
      else if (strcasecmp("media-source", keyword) == 0)
	stp_set_float_parameter(key.v, "Brightness", atof(value));
      else if (strcasecmp("scaling", keyword) == 0)
	key.scaling = atof(value);
      else if (strcasecmp("orientation", keyword) == 0)
	key.orientation = atoi(value);
      else if (strcasecmp("left", keyword) == 0)
	stp_set_left(key.v, atoi(value));
      else if (strcasecmp("top", keyword) == 0)
	stp_set_top(key.v, atoi(value));
      else if (strcasecmp("linear", keyword) == 0)
	/* Ignore linear */
	;
      else if (strcasecmp("image-type", keyword) == 0)
	/* Ignore image type */
	;
      else if (strcasecmp("unit", keyword) == 0)
	key.unit = atoi(value);
      else if (strcasecmp("custom-page-width", keyword) == 0)
	stp_set_page_width(key.v, atoi(value));
      else if (strcasecmp("custom-page-height", keyword) == 0)
	stp_set_page_height(key.v, atoi(value));
      /* Special case Ink-Type and Dither-Algorithm */
      else if (strcasecmp("ink-type", keyword) == 0)
	stp_set_string_parameter(key.v, "InkType", value);
      else if (strcasecmp("dither-algorithm", keyword) == 0)
	stp_set_string_parameter(key.v, "DitherAlgorithm", value);
      else
	{
	  stp_parameter_t desc;
	  stp_curve_t *curve;
	  stp_describe_parameter(key.v, keyword, &desc);
	  switch (desc.p_type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      stp_set_string_parameter(key.v, keyword, value);
	      break;
	    case STP_PARAMETER_TYPE_FILE:
	      stp_set_file_parameter(key.v, keyword, value);
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      stp_set_float_parameter(key.v, keyword, atof(value));
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      stp_set_int_parameter(key.v, keyword, atoi(value));
	      break;
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      stp_set_boolean_parameter(key.v, keyword, atoi(value));
	      break;
	    case STP_PARAMETER_TYPE_CURVE:
	      curve = stp_curve_create_from_string(value);
	      if (curve)
		{
		  stp_set_curve_parameter(key.v, keyword, curve);
		  stp_curve_destroy(curve);
		}
	      break;
	    default:
	      if (strlen(value))
		{
		  char buf[1024];
		  snprintf(buf, sizeof(buf),
			   "Unrecognized keyword `%s' in printrc; value `%s' (%d)\n",
			   keyword, value, desc.p_type);
		}
	    }
	  stp_parameter_description_destroy(&desc);
	}
    }
  if (strlen(key.name) > 0)
    {
      stpui_plist_add(&key, 0);
      stp_vars_destroy(key.v);
      free(key.name);
      free(key.output_to);
    }
  if (current_printer)
    {
      int i;
      for (i = 0; i < stpui_plist_count; i ++)
	if (strcmp(current_printer, stpui_plist[i].name) == 0)
	  stpui_plist_current = i;
    }
}  

const char *stpui_printrc_current_printer = NULL;
extern FILE *yyin;
extern int yyparse(void);

static void
stpui_printrc_load_v2(FILE *fp)
{
  int retval;
  yyin = fp;

  stpui_printrc_current_printer = NULL;
  retval = yyparse();
  if (stpui_printrc_current_printer)
    {
      int i;
      for (i = 0; i < stpui_plist_count; i ++)
	{
	  if (strcmp(stpui_printrc_current_printer, stpui_plist[i].name) == 0)
	    stpui_plist_current = i;
	  if (!stp_check_boolean_parameter(stpui_plist[i].v,
					   "PageSizeExtended",
					   STP_PARAMETER_ACTIVE))
	    stp_set_boolean_parameter(stpui_plist[i].v, "PageSizeExtended", 0);
	}
      STP_SAFE_FREE(stpui_printrc_current_printer);
    }
}

/*
 * 'stpui_printrc_load()' - Load the printer resource configuration file.
 */
void
stpui_printrc_load(void)
{
  FILE		*fp;		/* Printrc file */
  char		line[1024];	/* Line in printrc file */
  int		format = 0;	/* rc file format version */
  int		system_printers; /* printer count before reading printrc */
  const char *filename = stpui_get_printrc_file();

  check_plist(1);

 /*
  * Get the printer list...
  */

  stpui_get_system_printers();

  system_printers = stpui_plist_count - 1;

  if ((fp = fopen(filename, "r")) != NULL)
    {
      (void) memset(line, 0, 1024);
      if (fgets(line, sizeof(line), fp) != NULL)
	{
	  if (strncmp("#PRINTRCv", line, 9) == 0)
	    {
#ifdef DEBUG
	      fprintf(stderr, "Found printrc version tag: `%s'\n", line);
	      fprintf(stderr, "Version number: `%s'\n", &(line[9]));
#endif
	      (void) sscanf(&(line[9]), "%d", &format);
	    }
	}
      rewind(fp);
      switch (format)
	{
	case 0:
	  stpui_printrc_load_v0(fp);
	  break;
	case 1:
	  stpui_printrc_load_v1(fp);
	  break;
	case 2:
	  stpui_printrc_load_v2(fp);
	  break;
	}
      (void) fclose(fp);
    }
}

/*
 * 'stpui_printrc_save()' - Save the current printer resource configuration.
 */
void
stpui_printrc_save(void)
{
  FILE		*fp;		/* Printrc file */
  int		i;		/* Looping var */
  stpui_plist_t	*p;		/* Current printer */
  const char *filename = stpui_get_printrc_file();


  if ((fp = fopen(filename, "w")) != NULL)
    {
      /*
       * Write the contents of the printer list...
       */

#ifdef DEBUG
      fprintf(stderr, "Number of printers: %d\n", stpui_plist_count);
#endif

      fputs("#PRINTRCv2 written by GIMP-PRINT " PLUG_IN_VERSION "\n", fp);

      fprintf(fp, "Current-Printer: \"%s\"\n",
	      stpui_plist[stpui_plist_current].name);
      fprintf(fp, "Show-All-Paper-Sizes: %s\n",
	      stpui_show_all_paper_sizes ? "True" : "False");
      for (i = 0, p = stpui_plist; i < stpui_plist_count; i ++, p ++)
	{
	  int count;
	  int j;
	  stp_parameter_list_t *params = stp_get_parameter_list(p->v);
	  count = stp_parameter_list_count(params);
	  fprintf(fp, "\nPrinter: \"%s\" \"%s\"\n",
		  p->name, stp_get_driver(p->v));
	  fprintf(fp, "Destination: \"%s\"\n", stpui_plist_get_output_to(p));
	  fprintf(fp, "Scaling: %.3f\n", p->scaling);
	  fprintf(fp, "Orientation: %d\n", p->orientation);
	  fprintf(fp, "Autosize-Roll-Paper: %d\n", p->auto_size_roll_feed_paper);
	  fprintf(fp, "Unit: %d\n", p->unit);

	  fprintf(fp, "Left: %d\n", stp_get_left(p->v));
	  fprintf(fp, "Top: %d\n", stp_get_top(p->v));
	  fprintf(fp, "Custom_Page_Width: %d\n", stp_get_page_width(p->v));
	  fprintf(fp, "Custom_Page_Height: %d\n", stp_get_page_height(p->v));

	  for (j = 0; j < count; j++)
	    {
	      const stp_parameter_t *param = stp_parameter_list_param(params, j);
	      if (strcmp(param->name, "AppGamma") == 0)
		continue;
	      switch (param->p_type)
		{
		case STP_PARAMETER_TYPE_STRING_LIST:
		  if (stp_check_string_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "Parameter %s String %s \"%s\"\n",
			    param->name,
			    ((stp_get_string_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_string_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_FILE:
		  if (stp_check_file_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "Parameter %s File %s \"%s\"\n", param->name,
			    ((stp_get_file_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_file_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_DOUBLE:
		  if (stp_check_float_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "Parameter %s Double %s %f\n", param->name,
			    ((stp_get_float_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_float_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_INT:
		  if (stp_check_int_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "Parameter %s Int %s %d\n", param->name,
			    ((stp_get_int_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    stp_get_int_parameter(p->v, param->name));
		  break;
		case STP_PARAMETER_TYPE_BOOLEAN:
		  if (stp_check_boolean_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    fprintf(fp, "Parameter %s Boolean %s %s\n", param->name,
			    ((stp_get_boolean_parameter_active
			      (p->v, param->name) == STP_PARAMETER_ACTIVE) ?
			     "True" : "False"),
			    (stp_get_boolean_parameter(p->v, param->name) ?
			     "True" : "False"));
		  break;
		case STP_PARAMETER_TYPE_CURVE:
		  if (stp_check_curve_parameter(p->v, param->name,
						 STP_PARAMETER_INACTIVE))
		    {
		      const stp_curve_t *curve =
			stp_get_curve_parameter(p->v, param->name);
		      if (curve)
			{
			  fprintf(fp, "Parameter %s Curve %s '",
				  param->name,
				  ((stp_get_curve_parameter_active
				    (p->v, param->name) ==
				    STP_PARAMETER_ACTIVE) ?
				   "True" : "False"));
			  stp_curve_write(fp, curve);
			  fprintf(fp, "'\n");
			}
		    }
		  break;
		default:
		  break;
		}
	    }
	  stp_parameter_list_destroy(params);
#ifdef DEBUG
	  fprintf(stderr, "Wrote printer %d: %s\n", i, p->name);
#endif
	}
      fclose(fp);
    }
  else
    fprintf(stderr, "could not open printrc file \"%s\"\n",filename);
}

/*
 * 'compare_printers()' - Compare system printer names for qsort().
 */

static int
compare_printers(stpui_plist_t *p1, stpui_plist_t *p2)
{
  return (strcmp(p1->name, p2->name));
}

/*
 * 'stpui_get_system_printers()' - Get a complete list of printers from the spooler.
 */

#define PRINTERS_NONE	0
#define PRINTERS_LPC	1
#define PRINTERS_LPSTAT	2

void
stpui_get_system_printers(void)
{
  int   i;			/* Looping var */
  int	type;			/* 0 = none, 1 = lpc, 2 = lpstat */
  char	command[255];		/* Command to run */
  char  defname[128];		/* Default printer name */
  FILE *pfile;			/* Pipe to status command */
  char  line[255];		/* Line from status command */
  char	*ptr;			/* Pointer into line */
  char  name[128];		/* Printer name from status command */
  static const char	*lpcs[] =	/* Possible locations of LPC... */
		{
		  "/etc"
		  "/usr/bsd",
		  "/usr/etc",
		  "/usr/libexec",
		  "/usr/sbin"
		};

 /*
  * Setup defaults...
  */

  defname[0] = '\0';

  check_plist(1);
  stpui_plist_count = 1;
  stpui_plist_set_name(&(stpui_plist[0]), _("File"));
  stpui_plist[0].active = 1;
  stp_set_driver(stpui_plist[0].v, "ps2");
  stp_set_string_parameter(stpui_plist[0].v, "PrintingMode", "Color");

 /*
  * Figure out what command to run...  We use lpstat if it is available over
  * lpc since Solaris, CUPS, etc. provide both commands.  No need to list
  * each printer twice...
  */

  if (!access("/usr/bin/lpstat", X_OK))
  {
    strcpy(command, "/usr/bin/lpstat -d -p");
    type = PRINTERS_LPSTAT;
  }
  else
  {
    for (i = 0; i < (sizeof(lpcs) / sizeof(lpcs[0])); i ++)
    {
      sprintf(command, "%s/lpc", lpcs[i]);

      if (!access(command, X_OK))
        break;
    }

    if (i < (sizeof(lpcs) / sizeof(lpcs[0])))
    {
      strcat(command, " status < /dev/null");
      type = PRINTERS_LPC;
    }
    else
      type = PRINTERS_NONE;
  }

 /*
  * Run the command, if any, to get the available printers...
  */

  if (type > PRINTERS_NONE)
  {
    if ((pfile = popen(command, "r")) != NULL)
    {
     /*
      * Read input as needed...
      */

      while (fgets(line, sizeof(line), pfile) != NULL)
        switch (type)
	{
	  char *result;
	  case PRINTERS_LPC :
	      if (!strncmp(line, "Press RETURN to continue", 24) &&
		  (ptr = strchr(line, ':')) != NULL &&
		  (strlen(ptr) - 2) < (ptr - line))
		strcpy(line, ptr + 2);

	      if ((ptr = strchr(line, ':')) != NULL &&
	          line[0] != ' ' && line[0] != '\t')
              {
		int printer_exists = 0;
		*ptr = '\0';
                /* check for duplicate printers--yes, they can happen,
                 * and it makes gimp-print forget everything about the
                 * printer */
                for (i = 1; i < stpui_plist_count; i++)
                  if (strcmp(line, stpui_plist[i].name) == 0)
		    {
		      printer_exists = 1;
		      break;
		    }
		if (printer_exists)
		  break;

		check_plist(stpui_plist_count + 1);
		stpui_plist_set_name(&(stpui_plist[stpui_plist_count]), line);
#ifdef DEBUG
                fprintf(stderr, "Adding new printer from lpc: <%s>\n",
                  line);
#endif
		result = g_strdup_printf("lpr -l -P%s", line);
		stpui_plist_set_output_to(&(stpui_plist[stpui_plist_count]), result);
		free(result);
		stp_set_driver(stpui_plist[stpui_plist_count].v, "ps2");
		stpui_plist[stpui_plist_count].active = 1;
		stpui_plist_count ++;
	      }
	      break;

	  case PRINTERS_LPSTAT :
	      if ((sscanf(line, "printer %127s", name) == 1) ||
		  (sscanf(line, "Printer: %127s", name) == 1))
	      {
		int printer_exists = 0;
                /* check for duplicate printers--yes, they can happen,
                 * and it makes gimp-print forget everything about the
                 * printer */
                for (i = 1; i < stpui_plist_count; i++)
                  if (strcmp(name, stpui_plist[i].name) == 0)
		    {
		      printer_exists = 1;
		      break;
		    }
		if (printer_exists)
		  break;
		check_plist(stpui_plist_count + 1);
		stpui_plist_set_name(&(stpui_plist[stpui_plist_count]), name);
#ifdef DEBUG
                fprintf(stderr, "Adding new printer from lpc: <%s>\n",
                  name);
#endif
		result = g_strdup_printf("lp -oraw -s -d%s", name);
		stpui_plist_set_output_to(&(stpui_plist[stpui_plist_count]), result);
		free(result);
		stp_set_driver(stpui_plist[stpui_plist_count].v, "ps2");
		stpui_plist[stpui_plist_count].active = 1;
        	stpui_plist_count ++;
	      }
	      else
        	sscanf(line, "system default destination: %127s", defname);
	      break;
	}

      pclose(pfile);
    }
  }

  if (stpui_plist_count > 2)
    qsort(stpui_plist + 1, stpui_plist_count - 1, sizeof(stpui_plist_t),
          (int (*)(const void *, const void *))compare_printers);

  if (defname[0] != '\0')
  {
    for (i = 0; i < stpui_plist_count; i ++)
      if (strcmp(defname, stpui_plist[i].name) == 0)
        break;

    if (i < stpui_plist_count)
      stpui_plist_current = i;
  }
}

const stpui_plist_t *
stpui_get_current_printer(void)
{
  return &(stpui_plist[stpui_plist_current]);
}

/*
 * 'usr1_handler()' - Make a note when we receive SIGUSR1.
 */

static volatile int usr1_interrupt;

static void
usr1_handler (int sig)
{
  usr1_interrupt = 1;
}

static void
writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

static void
stpui_errfunc(void *file, const char *buf, size_t bytes)
{
  g_message(buf);
}

int
stpui_print(const stpui_plist_t *printer, stpui_image_t *image)
{
  int		ppid = getpid (), /* PID of plugin */
		opid,		/* PID of output process */
		cpid = 0,	/* PID of control/monitor process */
		pipefd[2],	/* Fds of the pipe connecting all the above */
		errfd[2],	/* Message logger from lp command */
		syncfd[2];	/* Sync the logger */
  FILE		*prn = NULL;	/* Print file/command */
  int		do_sync = 0;
  int		dummy;

  /*
   * Open the file/execute the print command...
   */

  if (stpui_plist_current > 0)
    {
      /*
       * The following IPC code is only necessary because the GIMP kills
       * plugins with SIGKILL if its "Cancel" button is pressed; this
       * gives the plugin no chance whatsoever to clean up after itself.
       */
      do_sync = 1;
      usr1_interrupt = 0;
      signal (SIGUSR1, usr1_handler);
      if (pipe (syncfd) != 0)
	{
	  do_sync = 0;
	}
      if (pipe (pipefd) != 0)
	{
	  prn = NULL;
	}
      else
	{
	  cpid = fork ();
	  if (cpid < 0)		/* Error */
	    {
	      do_sync = 0;
	      prn = NULL;
	    }
	  else if (cpid == 0)	/* Child 1 (lpr monitor) */
	    {
	      /* LPR monitor process.  Printer output is piped to us. */
	      close(syncfd[0]);
	      opid = fork ();
	      if (opid < 0)
		{
		  /* Errors will cause the plugin to get a SIGPIPE.  */
		  exit (1);
		}
	      else if (opid == 0) /* Child 2 (printer command) */
		{
		  dup2 (pipefd[0], 0);
		  close (pipefd[0]);
		  close (pipefd[1]);
		  if (pipe(errfd) == 0)
		    {
		      opid = fork();
		      if (opid < 0)
			_exit(1);
		      else if (opid == 0) /* Child 3 (monitors stderr) */
			{ 
			  stp_outfunc_t errfunc = stpui_get_errfunc();
			  void *errdata = stpui_get_errdata();
			  /* calls g_message on anything it sees */
			  char buf[4096];

			  close (pipefd[0]);
			  close (pipefd[1]);
			  while (1)
			    {
			      ssize_t bytes = read(errfd[0], buf, 4095);
			      if (bytes > 0)
				{
				  buf[bytes] = '\0';
				  (*errfunc)(errdata, buf, bytes);
				}
			      else
				{
				  if (bytes < 0)
				    {
				      snprintf(buf, 4095,
					       "Read messages failed: %s\n",
					       strerror(errno));
				      (*errfunc)(errdata, buf, strlen(buf));
				    }
				  write(syncfd[1], "Done", 5);
				  _exit(0);
				}
			    }
			  /* NOTREACHED */
			  write(syncfd[1], "Done", 5);
			  _exit(0);
			}
		      else	/* Child 2 (printer command) */
			{
			  (void) close(2);
			  (void) close(1);
			  dup2 (errfd[1], 2);
			  dup2 (errfd[1], 1);
			  close(errfd[1]);
			  close (pipefd[0]);
			  close (pipefd[1]);
			  close(syncfd[1]);
			  execl("/bin/sh", "/bin/sh", "-c",
				stpui_plist_get_output_to(printer), NULL);
			  /* NOTREACHED */
			  _exit (1);
			}
		      /* NOTREACHED */
		      _exit(1);
		    }
		  else		/* pipe() failed! */
		    {
		      _exit(1);
		    }
		}
	      else		/* Child 1 (lpr monitor) */
		{
		  /*
		   * If the print plugin gets SIGKILLed by gimp, we kill lpr
		   * in turn.  If the plugin signals us with SIGUSR1 that it's
		   * finished printing normally, we close our end of the pipe,
		   * and go away.
		   */
		  close (0);
		  close (1);
		  close (2);
		  close (syncfd[1]);
		  close (pipefd[0]);
		  while (usr1_interrupt == 0)
		    {
		      if (kill (ppid, 0) < 0)
			{
			  /* The print plugin has been killed!  */
			  kill (opid, SIGTERM);
			  waitpid (opid, &dummy, 0);
			  close (pipefd[1]);
			  /*
			   * We do not want to allow cleanup before exiting.
			   * The exiting parent has already closed the
			   * connection  to the X server; if we try to clean
			   * up, we'll notice that fact and complain.
			   */
			  _exit (0);
			}
		      sleep (5);
		    }
		  /* We got SIGUSR1.  */
		  close (pipefd[1]);
		  /*
		   * We do not want to allow cleanup before exiting.
		   * The exiting parent has already closed the connection
		   * to the X server; if we try to clean up, we'll notice
		   * that fact and complain.
		   */
		  _exit (0);
		}
	    }
	  else			/* Parent (actually generates the output) */
	    {
	      close (syncfd[1]);
	      close (pipefd[0]);
	      /* Parent process.  We generate the printer output. */
	      prn = fdopen (pipefd[1], "w");
	      /* and fall through... */
	    }
	}
    }
  else
    prn = fopen (stpui_plist_get_output_to(printer), "wb");

  if (prn != NULL)
    {
      char tmp[32];
      stpui_plist_t *np = allocate_stpui_plist_copy(printer);
      const stp_vars_t *current_vars =
	stp_printer_get_defaults(stp_get_printer(np->v));
      int orientation;
      stp_merge_printvars(np->v, current_vars);
      stp_set_string_parameter(np->v, "InputImageType", image_type);
      if (image_raw_channels)
	{
	  sprintf(tmp, "%d", image_raw_channels);
	  stp_set_string_parameter(np->v, "RawChannels", tmp);
	}
      sprintf(tmp, "%d", image_channel_depth);
      stp_set_string_parameter(np->v, "ChannelBitDepth", tmp);

      /*
       * Set up the orientation
       */
      orientation = np->orientation;
      if (orientation == ORIENT_AUTO)
	orientation = stpui_compute_orientation();
      switch (orientation)
	{
	case ORIENT_PORTRAIT:
	  break;
	case ORIENT_LANDSCAPE:
	  if (image->rotate_cw)
	    (image->rotate_cw)(image);
	  break;
	case ORIENT_UPSIDEDOWN:
	  if (image->rotate_180)
	    (image->rotate_180)(image);
	  break;
	case ORIENT_SEASCAPE:
	  if (image->rotate_ccw)
	    (image->rotate_ccw)(image);
	  break;
	}

      /*
       * Finally, call the print driver to send the image to the printer
       * and close the output file/command...
       */

      stp_set_outfunc(np->v, writefunc);
      stp_set_errfunc(np->v, stpui_get_errfunc());
      stp_set_outdata(np->v, prn);
      stp_set_errdata(np->v, stpui_get_errdata());
      if (stp_print(np->v, &(image->im)) != 1)
	{
	  stpui_plist_destroy(np);
	  g_free(np);
	  return 0;
	}

      if (stpui_plist_current > 0)
	{
	  fclose (prn);
	  kill (cpid, SIGUSR1);
	  waitpid (cpid, &dummy, 0);
	}
      else
	fclose (prn);
      if (do_sync)
	{
	  char buf[8];
	  (void) read(syncfd[0], buf, 8);
	}
      stpui_plist_destroy(np);
      g_free(np);
      return 1;
    }
  return 0;
}

/*
 * End of "$Id$".
 */
