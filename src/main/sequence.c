/*
 * "$Id$"
 *
 *   Sequence data type.  This type is designed to be derived from by
 *   the curve and dither matrix types.
 *
 *   Copyright 2002-2003 Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2003      Roger Leigh (rleigh@debian.org)
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
#include <gimp-print/sequence.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include "xml.h"

#define COOKIE_SEQUENCE 0xbf6a28d2

typedef struct
{
  int cookie;
  int recompute_range; /* Do we need to recompute the min and max? */
  double blo;          /* Lower bound */
  double bhi;          /* Upper bound */
  double rlo;          /* Lower range limit */
  double rhi;          /* Upper range limit */
  size_t size;         /* Number of points */
  double *data;        /* Array of doubles */
  float *float_data;   /* Data converted to other form */
  long *long_data;
  unsigned long *ulong_data;
  int *int_data;
  unsigned *uint_data;
  short *short_data;
  unsigned short *ushort_data;
} stpi_internal_sequence_t;

/*
 * We could do more sanity checks here if we want.
 */
static inline void
check_sequence(const stpi_internal_sequence_t *v)
{
  if (v == NULL)
    {
      stpi_erprintf("Null stp_sequence_t! Please report this bug.\n");
      stpi_abort();
    }
  if (v->cookie != COOKIE_SEQUENCE)
    {
      stpi_erprintf("Bad stp_sequence_t! Please report this bug.\n");
      stpi_abort();
    }
}

static void
sequence_ctor(stpi_internal_sequence_t *iseq)
{
  iseq->cookie = COOKIE_SEQUENCE;
  iseq->rlo = iseq->blo = 0.0;
  iseq->rhi = iseq->bhi = 1.0;
  iseq->recompute_range = 1;
  iseq->size = 0;
  iseq->data = NULL;
}

stp_sequence_t
stp_sequence_create(void)
{
  stpi_internal_sequence_t *ret;
  ret = stpi_zalloc(sizeof(stpi_internal_sequence_t));
  sequence_ctor(ret);
  return (stp_sequence_t) ret;
}

static void
invalidate_auxilliary_data(stpi_internal_sequence_t *iseq)
{
  SAFE_FREE(iseq->float_data);
  SAFE_FREE(iseq->long_data);
  SAFE_FREE(iseq->ulong_data);
  SAFE_FREE(iseq->int_data);
  SAFE_FREE(iseq->uint_data);
  SAFE_FREE(iseq->short_data);
  SAFE_FREE(iseq->ushort_data);
}

static void
sequence_dtor(stpi_internal_sequence_t *iseq)
{
  invalidate_auxilliary_data(iseq);
  if (iseq->data)
    stpi_free(iseq->data);
  memset(iseq, 0, sizeof(stpi_internal_sequence_t));
}

void
stp_sequence_destroy(stp_sequence_t sequence)
{
  stpi_internal_sequence_t *iseq = (stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  sequence_dtor(iseq);
  stpi_free(iseq);
}

void
stp_sequence_copy(stp_sequence_t dest, stp_const_sequence_t source)
{
  stpi_internal_sequence_t *idest = (stpi_internal_sequence_t *) dest;
  const stpi_internal_sequence_t *isource =
    (const stpi_internal_sequence_t *) source;
  check_sequence(idest);
  check_sequence(isource);

  idest->recompute_range = isource->recompute_range;
  idest->blo = isource->blo;
  idest->bhi = isource->bhi;
  idest->rlo = isource->rlo;
  idest->rhi = isource->rhi;
  idest->size = isource->size;
  idest->data = stpi_zalloc(sizeof(double) * isource->size);
  memcpy(idest->data, isource->data, (sizeof(double) * isource->size));
}

stp_sequence_t
stp_sequence_create_copy(stp_const_sequence_t sequence)
{
  const stpi_internal_sequence_t *iseq =
    (const stpi_internal_sequence_t *) sequence;
  stp_sequence_t ret;
  check_sequence(iseq);
  ret = stp_sequence_create();
  stp_sequence_copy(ret, sequence);
  return ret;
}

int
stp_sequence_set_bounds(stp_sequence_t sequence, double low, double high)
{
  stpi_internal_sequence_t *iseq = (stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  if (low > high)
    return 0;
  iseq->rlo = iseq->blo = low;
  iseq->rhi = iseq->bhi = high;
  iseq->recompute_range = 1;
  return 1;
}

void
stp_sequence_get_bounds(stp_const_sequence_t sequence,
			double *low, double *high)
{
  const stpi_internal_sequence_t *iseq =
    (const stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  *low = iseq->blo;
  *high = iseq->bhi;
}


static stpi_internal_sequence_t *
cast_to_iseq(stp_const_sequence_t sequence)
{
  stpi_internal_sequence_t *answer = (stpi_internal_sequence_t *) sequence;
  check_sequence(answer);
  return answer;
}

/*
 * Find the minimum and maximum points on the curve.
 */
static void
scan_sequence_range(stpi_internal_sequence_t *seq)
{
  int i;
  seq->rlo = seq->bhi;
  seq->rhi = seq->blo;
  if (seq->size)
    for (i = 0; i < seq->size; i++)
      {
	if (seq->data[i] < seq->rlo)
	  seq->rlo = seq->data[i];
	if (seq->data[i] > seq->rhi)
	  seq->rhi = seq->data[i];
      }
  seq->recompute_range = 0; /* Don't recompute unless the data changes */
}

void
stp_sequence_get_range(stp_const_sequence_t sequence,
		       double *low, double *high)
{
  stpi_internal_sequence_t *iseq = cast_to_iseq(sequence);
  if (iseq->recompute_range) /* Don't recompute the range if we don't
			       need to. */
    scan_sequence_range(iseq);
  *low = iseq->rlo;
  *high = iseq->rhi;
}


int
stp_sequence_set_size(stp_sequence_t sequence, size_t size)
{
  stpi_internal_sequence_t *iseq = (stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  if (iseq->data) /* Free old data */
    {
      stpi_free(iseq->data);
      iseq->data = NULL;
    }
  iseq->size = size;
  iseq->recompute_range = 1; /* Always recompute on change */
  if (size == 0)
    return 1;
  invalidate_auxilliary_data(iseq);
  iseq->data = stpi_zalloc(sizeof(double) * size);
  return 1;
}


size_t
stp_sequence_get_size(stp_const_sequence_t sequence)
{
  const stpi_internal_sequence_t *iseq =
    (const stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  return iseq->size;
}



int
stp_sequence_set_data(stp_sequence_t sequence,
		      size_t size, const double *data)
{
  stpi_internal_sequence_t *iseq = (stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  iseq->size = size;
  if (iseq->data)
    stpi_free(iseq->data);
  iseq->data = stpi_zalloc(sizeof(double) * size);
  memcpy(iseq->data, data, (sizeof(double) * size));
  invalidate_auxilliary_data(iseq);
  iseq->recompute_range = 1;
  return 1;
}

int
stp_sequence_set_subrange(stp_sequence_t sequence, size_t where,
			  size_t size, const double *data)
{
  stpi_internal_sequence_t *iseq = (stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  if (where + size > iseq->size) /* Exceeds data size */
    return 0;
  memcpy(iseq->data+where, data, (sizeof(double) * size));
  invalidate_auxilliary_data(iseq);
  iseq->recompute_range = 1;
  return 1;
}


void
stp_sequence_get_data(stp_const_sequence_t sequence, size_t *size,
		      const double **data)
{
  const stpi_internal_sequence_t *iseq =
    (const stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);
  *size = iseq->size;
  *data = iseq->data;
}


int
stp_sequence_set_point(stp_sequence_t sequence, size_t where,
		       double data)
{
  stpi_internal_sequence_t *iseq = (stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);

  if (where >= iseq->size || ! finite(data) ||
      data < iseq->blo || data > iseq->bhi)
    return 0;

  if (iseq->recompute_range == 0 && (data < iseq->rlo ||
				     data > iseq->rhi ||
				     iseq->data[where] == iseq->rhi ||
				     iseq->data[where] == iseq->rlo))
    iseq->recompute_range = 1;

  iseq->data[where] = data;
  invalidate_auxilliary_data(iseq);
  return 1;
}

int
stp_sequence_get_point(stp_const_sequence_t sequence, size_t where,
		       double *data)
{
  const stpi_internal_sequence_t *iseq =
    (const stpi_internal_sequence_t *) sequence;
  check_sequence(iseq);

  if (where >= iseq->size)
    return 0;
  *data = iseq->data[where];
  return 1;
}

stp_sequence_t
stpi_sequence_create_from_xmltree(mxml_node_t *da)
{
  const char *stmp;
  stp_sequence_t ret = NULL;
  size_t point_count;
  double low, high;
  int i;

  ret = stp_sequence_create();

  /* Get curve point count */
  stmp = stpi_mxmlElementGetAttr(da, "count");
  if (stmp)
    {
      point_count = (size_t) stpi_xmlstrtoul(stmp);
      if ((stpi_xmlstrtol(stmp)) < 0)
	{
	  stpi_erprintf("stpi_sequence_create_from_xmltree: \"count\" is less than zero\n");
	  goto error;
	}
    }
  else
    {
      stpi_erprintf("stpi_sequence_create_from_xmltree: \"count\" missing\n");
      goto error;
    }
  /* Get lower bound */
  stmp = stpi_mxmlElementGetAttr(da, "lower-bound");
  if (stmp)
    {
      low = stpi_xmlstrtod(stmp);
    }
  else
    {
      stpi_erprintf("stpi_sequence_create_from_xmltree: \"lower-bound\" missing\n");
      goto error;
    }
  /* Get upper bound */
  stmp = stpi_mxmlElementGetAttr(da, "upper-bound");
  if (stmp)
    {
      high = stpi_xmlstrtod(stmp);
    }
  else
    {
      stpi_erprintf("stpi_sequence_create_from_xmltree: \"upper-bound\" missing\n");
      goto error;
    }

  stpi_deprintf(STPI_DBG_XML,
		"stpi_sequence_create_from_xmltree: stp_sequence_set_size: %d\n",
		point_count);
  stp_sequence_set_size(ret, point_count);
  stp_sequence_set_bounds(ret, low, high);

  /* Now read in the data points */
  if (point_count)
    {
      mxml_node_t *child = da->child;
      i = 0;
      while (child && i < point_count)
	{
	  if (child->type == MXML_TEXT)
	    {
	      char *endptr;
	      double tmpval = strtod(child->value.text.string, &endptr);
	      if (endptr == child->value.text.string)
		{
		  stpi_erprintf
		    ("stpi_sequence_create_from_xmltree: bad data %s\n",
		     child->value.text.string);
		  goto error;
		}
	      if (! finite(tmpval)
		  || ( tmpval == 0 && errno == ERANGE )
		  || tmpval < low
		  || tmpval > high)
		{
		  stpi_erprintf("stpi_sequence_create_from_xmltree: "
				"read aborted: datum out of bounds: "
				"%g (require %g <= x <= %g), n = %d\n",
				tmpval, low, high, i);
		  goto error;
		}
	      /* Datum was valid, so now add to the sequence */
	      stp_sequence_set_point(ret, i, tmpval);
	      i++;
	      child = child->next;
	    }
	}
      if (i < point_count)
	{
	  stpi_erprintf("stpi_sequence_create_from_xmltree: "
			"read aborted: too little data "
			"(n=%d, needed %d)\n", i, point_count);
	  goto error;
	}	
    }

  return ret;

 error:
  stpi_erprintf("stpi_sequence_create_from_xmltree: error during sequence read\n");
  if (ret)
    stp_sequence_destroy(ret);
  return NULL;
}

mxml_node_t *
stpi_xmltree_create_from_sequence(stp_sequence_t seq)   /* The sequence */
{
  size_t pointcount;
  double low;
  double high;

  char *count;
  char *lower_bound;
  char *upper_bound;

  mxml_node_t *seqnode;

  int i;                 /* loop counter */

  pointcount = stp_sequence_get_size(seq);
  stp_sequence_get_bounds(seq, &low, &high);

  /* should count be of greater precision? */
  stpi_asprintf(&count, "%lu", (unsigned long) pointcount);
  stpi_asprintf(&lower_bound, "%g", low);
  stpi_asprintf(&upper_bound, "%g", high);

  seqnode = stpi_mxmlNewElement(NULL, "sequence");
  (void) stpi_mxmlElementSetAttr(seqnode, "count", count);
  (void) stpi_mxmlElementSetAttr(seqnode, "lower-bound", lower_bound);
  (void) stpi_mxmlElementSetAttr(seqnode, "upper-bound", upper_bound);

  stpi_free(count);
  stpi_free(lower_bound);
  stpi_free(upper_bound);

  /* Write the curve points into the node content */
  if (pointcount) /* Is there any data to write? */
    {
      for (i = 0; i < pointcount; i++)
	{
	  double dval;
	  char *sval;

	  if ((stp_sequence_get_point(seq, i, &dval)) != 1)
	    goto error;

	  stpi_asprintf(&sval, "%g", dval);
	  stpi_mxmlNewText(seqnode, 1, sval);
	  stpi_free(sval);
      }
    }
  return seqnode;

 error:
  if (seqnode)
    stpi_mxmlDelete(seqnode);
  return NULL;
}


/* "Overloaded" functions */

#define DEFINE_DATA_SETTER(t, name)					     \
int									     \
stp_sequence_set_##name##_data(stp_sequence_t sequence,                      \
                               size_t count, const t *data)                  \
{									     \
  int i;								     \
  stpi_internal_sequence_t *iseq = (stpi_internal_sequence_t *) sequence;    \
  check_sequence(iseq);						             \
  if (count < 2)							     \
    return 0;								     \
									     \
  /* Valiseqte the data before we commit to it. */			     \
  for (i = 0; i < count; i++)						     \
    if (! finite(data[i]) || data[i] < iseq->blo || data[i] > iseq->bhi)     \
      return 0;								     \
  stp_sequence_set_size(sequence, count);                                    \
  for (i = 0; i < count; i++)						     \
    stp_sequence_set_point(sequence, i, (double) data[i]);                   \
  return 1;								     \
}

DEFINE_DATA_SETTER(float, float)
DEFINE_DATA_SETTER(long, long)
DEFINE_DATA_SETTER(unsigned long, ulong)
DEFINE_DATA_SETTER(int, int)
DEFINE_DATA_SETTER(unsigned int, uint)
DEFINE_DATA_SETTER(short, short)
DEFINE_DATA_SETTER(unsigned short, ushort)


#define DEFINE_DATA_ACCESSOR(t, lb, ub, name)				     \
const t *								     \
stp_sequence_get_##name##_data(stp_const_sequence_t sequence, size_t *count) \
{									     \
  int i;								     \
  stpi_internal_sequence_t *iseq = cast_to_iseq(sequence);		     \
  if (iseq->blo < (double) lb || iseq->bhi > (double) ub)		     \
    return NULL;							     \
  if (!iseq->name##_data)						     \
    {									     \
      (iseq)->name##_data = stpi_zalloc(sizeof(t) * iseq->size);	     \
      for (i = 0; i < iseq->size; i++)					     \
	iseq->name##_data[i] = (t) iseq->data[i];			     \
    }									     \
  *count = iseq->size;							     \
  return iseq->name##_data;						     \
}

#ifndef HUGE_VALF /* ISO constant, from <math.h> */
#define HUGE_VALF 3.402823466E+38F
#endif

DEFINE_DATA_ACCESSOR(float, -HUGE_VALF, HUGE_VALF, float)
DEFINE_DATA_ACCESSOR(long, LONG_MIN, LONG_MAX, long)
DEFINE_DATA_ACCESSOR(unsigned long, 0, ULONG_MAX, ulong)
DEFINE_DATA_ACCESSOR(int, INT_MIN, INT_MAX, int)
DEFINE_DATA_ACCESSOR(unsigned int, 0, UINT_MAX, uint)
DEFINE_DATA_ACCESSOR(short, SHRT_MIN, SHRT_MAX, short)
DEFINE_DATA_ACCESSOR(unsigned short, 0, USHRT_MAX, ushort)
