/*
 * "$Id$"
 *
 *   Copyright 2002 Robert Krawitz (rlk@alum.mit.edu)
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef __GNU_LIBRARY__
#include <getopt.h>
#endif

#define DEBUG_SIGNAL
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif

int global_test_count = 0;
int global_error_count = 0;
int verbose = 0;
int quiet = 0;

#ifdef __GNU_LIBRARY__

struct option optlist[] =
{
  { "quiet",		0,	NULL,	(int) 'q' },
  { "verbose",		0,	NULL,	(int) 'v' },
  { NULL,		0,	NULL,	0 	  }
};
#endif

static void
TEST(const char *name)
{
  global_test_count++;
  printf("%d: Checking %s... ", global_test_count, name);
}

static void
TEST_PASS(void)
{
  fprintf(stdout, "PASS\n");
}

static void
TEST_FAIL(void)
{
  global_error_count++;
  fprintf(stdout, "FAIL\n");
}

static const double standard_sat_adjustment[] =
{
  0.50, 0.6,  0.7,  0.8,  0.9,  0.86, 0.82, 0.79, /* C */
  0.78, 0.8,  0.83, 0.87, 0.9,  0.95, 1.05, 1.15, /* B */
  1.3,  1.25, 1.2,  1.15, 1.12, 1.09, 1.06, 1.03, /* M */
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0, /* R */
  1.0,  0.9,  0.8,  0.7,  0.65, 0.6,  0.55, 0.52, /* Y */
  0.48, 0.47, 0.47, 0.49, 0.49, 0.49, 0.52, 0.51, /* G */
};

const char *good_curves[] =
  {
    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gimp-print xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gimpprint.xsd\">\n"
    "  <curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
    "    <sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87\n"
    "0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03\n"
    "1 1 1 1 1 1 1 1 1 0.9 0.8 0.7\n"
    "0.65 0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "  </curve>\n"
    "</gimp-print>\n",
    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gimp-print xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gimpprint.xsd\">\n"
    "  <curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\">\n"
    "    <sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87\n"
    "0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03\n"
    "1 1 1 1 1 1 1 1 1 0.9 0.8 0.7\n"
    "0.65 0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "  </curve>\n"
    "</gimp-print>\n",
    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gimp-print xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gimpprint.xsd\">\n"
    "  <curve wrap=\"wrap\" type=\"spline\" gamma=\"0\">\n"
    "    <sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87\n"
    "0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03\n"
    "1 1 1 1 1 1 1 1 1 0.9 0.8 0.7\n"
    "0.65 0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "  </curve>\n"
    "</gimp-print>\n",
    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gimp-print xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gimpprint.xsd\">\n"
    "  <curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
    "    <sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87\n"
    "0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03\n"
    "1 1 1 1 1 1 1 1 1 0.9 0.8 0.7\n"
    "0.65 0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "  </curve>\n"
    "</gimp-print>\n",
    /* Gamma curve 1 */
    "<?xml version=\"1.0\"?>\n"
    "<gimp-print xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gimpprint.xsd\">\n"
    "  <curve wrap=\"nowrap\" type=\"linear\" gamma=\"1\">\n"
    "    <sequence count=\"0\" lower-bound=\"0\" upper-bound=\"4\"/>\n"
    "  </curve>\n"
    "</gimp-print>\n",
    /* Gamma curve 2 */
    "<?xml version=\"1.0\"?>\n"
    "<gimp-print xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gimpprint.xsd\">\n"
    "  <curve wrap=\"nowrap\" type=\"linear\" gamma=\"1\">\n"
    "    <sequence count=\"0\" lower-bound=\"0\" upper-bound=\"4\"/>\n"
    "  </curve>\n"
    "</gimp-print>\n"
  };

static const int good_curve_count = sizeof(good_curves) / sizeof(const char *);

const char *bad_curves[] =
  {
    /* Bad point count */
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gimp-print><curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
    "<sequence count=\"-1\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8\n"
    "0.83 0.87 0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15\n"
    "1.12 1.09 1.06 1.03 1 1 1 1 1 1\n"
    "1 1 1 0.9 0.8 0.7 0.65 0.6 0.55 0.52\n"
    "0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence></curve></gimp-print>\n",
    /* Bad point count */
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gimp-print><curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
    "<sequence count=\"200\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8\n"
    "0.83 0.87 0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15\n"
    "1.12 1.09 1.06 1.03 1 1 1 1 1 1\n"
    "1 1 1 0.9 0.8 0.7 0.65 0.6 0.55 0.52\n"
    "0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence></curve></gimp-print>\n",
    /* Gamma curves */
    /* Incorrect wrap mode */
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gimp-print><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\">\n"
    "<sequence count=\"-1\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gimp-print>\n",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gimp-print><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\">\n"
    "<sequence count=\"1\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gimp-print>\n",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gimp-print><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\">\n"
    "<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gimp-print>\n",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gimp-print><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\">\n"
    "<sequence count=\"0\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gimp-print>\n"
  };

static const int bad_curve_count = sizeof(bad_curves) / sizeof(const char *);

const char *linear_curve_1 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print><curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gimp-print>";

const char *linear_curve_2 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print><curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gimp-print>";

const char *spline_curve_1 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print><curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gimp-print>";

const char *spline_curve_2 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print><curve wrap=\"wrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gimp-print>";


int
main(int argc, char **argv)
{
  char *tmp;
  int i;

  stp_curve_t curve1;
  stp_curve_t curve2;
  stp_curve_t curve3;

  while (1)
    {
#ifdef __GNU_LIBRARY__
      int option_index = 0;
      int c = getopt_long(argc, argv, "qv", optlist, &option_index);
#else
      int c = getopt(argc, argv, "qv");
#endif
      if (c == -1)
	break;
      switch (c)
	{
	case 'q':
	  quiet = 1;
	  verbose = 0;
	  break;
	case 'v':
	  quiet = 0;
	  verbose = 1;
	  break;
	default:
	  break;
	}
    }

  stp_init();

  TEST("creation of XML string from curve");
  curve1 = stp_curve_create(STP_CURVE_WRAP_AROUND);
  stp_curve_set_bounds(curve1, 0.0, 4.0);
  stp_curve_set_data(curve1, 48, standard_sat_adjustment);
  tmp = stp_curve_write_string(curve1);
  stp_curve_free(curve1);
  curve1 = NULL;
  if (tmp)
    TEST_PASS();
  else
    TEST_FAIL();
  if (verbose)
    fprintf(stdout, "%s\n", tmp);


  TEST("creation of curve from XML string (stp_curve_create_from_string)");
  if ((curve2 = stp_curve_create_from_string((const char *) tmp)) == NULL)
    TEST_FAIL();
  else
    TEST_PASS();
  free(tmp);

  TEST("stp_curve_resample");
  if (curve2 != NULL && stp_curve_resample(curve2, 95) == 0)
    {
      TEST_FAIL();
    }
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve2);
    }
  if (curve2)
    {
      stp_curve_free(curve2);
      curve2 = NULL;
    }

  if (!quiet)
    printf("Testing known bad curves...\n");
  for (i = 0; i < bad_curve_count; i++)
    {
      stp_curve_t bad = NULL;
      TEST("BAD curve (PASS is an expected failure)");
      if ((bad = stp_curve_create_from_string(bad_curves[i])) != NULL)
	{
	  TEST_FAIL();
	  if (!quiet)
	    {
	      stp_curve_write(stdout, bad);
	      fprintf(stdout, "\n");
	    }
	  stp_curve_free(bad);
	  bad = NULL;
	}
      else
	TEST_PASS();
    }

  if (!quiet)
    printf("Testing known good curves...\n");
  for (i = 0; i < good_curve_count; i++)
    {
      if (curve2)
	{
	  stp_curve_free(curve2);
	  curve2 = NULL;
	}
      TEST("GOOD curve");
      if ((curve2 = stp_curve_create_from_string(good_curves[i])) != NULL)
	{
	  TEST_PASS();
	  tmp = stp_curve_write_string(curve2);
	  TEST("whether XML curve is identical to original");
	  if (tmp && strcmp((const char *) tmp, good_curves[i]))
	    {
	      TEST_FAIL();
	      if (!quiet)
		{
		  printf("%s", tmp);
		  printf("%s", good_curves[i]);
		}
	    }
	  else
	    {
	      TEST_PASS();
	      if (verbose)
		printf("%s", tmp);
	    }
	  free(tmp);
	}
      else
	TEST_FAIL();
    }
  if (curve2)
    {
      stp_curve_free(curve2);
      curve2 = NULL;
    }
  if (verbose)
    printf("Allocate 1\n");
  curve1 = stp_curve_create(STP_CURVE_WRAP_NONE);
  if (verbose)
    printf("Allocate 2\n");
  curve2 = stp_curve_create(STP_CURVE_WRAP_NONE);
  TEST("set curve 1 gamma");
  if (!stp_curve_set_gamma(curve1, 1.2))
    TEST_FAIL();
  else
    TEST_PASS();
  if (verbose)
    stp_curve_write(stdout, curve1);
  TEST("set curve 2 gamma");
  if (!stp_curve_set_gamma(curve2, -1.2))
    TEST_FAIL();
  else
    TEST_PASS();
  if (verbose)
    stp_curve_write(stdout, curve2);

  TEST("compose add from gamma curves");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_ADD, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve3);
    }

  TEST("resample curve 1");
  if (!stp_curve_resample(curve1, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve1);
    }
  /*  if (curve3)
    {
      stp_curve_free(curve3);
      curve3 = NULL;
      }*/
  TEST("compose multiply from gamma curves");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	{
	  stp_curve_write(stdout, curve1);
	  stp_curve_write(stdout, curve2);
	  stp_curve_write(stdout, curve3);
	}
    }

  TEST("compose add from non-gamma curves");
  curve1 = stp_curve_create_from_string(good_curves[0]);
  curve2 = stp_curve_create_from_string(linear_curve_2);
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_ADD, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve3);
    }

  TEST("resample curve 1");
  if (!stp_curve_resample(curve1, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve1);
    }
  /*  if (curve3)
    {
      stp_curve_free(curve3);
      curve3 = NULL;
      }*/
  TEST("compose multiply from non-gamma curves");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	{
	  stp_curve_write(stdout, curve1);
	  stp_curve_write(stdout, curve2);
	  stp_curve_write(stdout, curve3);
	}
    }


  TEST("multiply rescale");
  if (!stp_curve_rescale(curve2, -1, STP_CURVE_COMPOSE_MULTIPLY,
			 STP_CURVE_BOUNDS_RESCALE))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if(verbose)
	stp_curve_write(stdout, curve2);
    }
  TEST("subtract compose");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_ADD, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve3);
    }

  /*  if (curve3)
    {
      stp_curve_free(curve3);
      curve3 = NULL;
      }*/
  if (curve1)
    {
      stp_curve_free(curve1);
      curve1 = NULL;
    }
  if (curve2)
    {
      stp_curve_free(curve2);
      curve2 = NULL;
    }

  curve1 = stp_curve_create(STP_CURVE_WRAP_NONE);
  curve2 = stp_curve_create(STP_CURVE_WRAP_AROUND);
  TEST("spline curve 1 creation");
  if ((curve1 =stp_curve_create_from_string(spline_curve_1)) == NULL)
    TEST_FAIL();
  else
    TEST_PASS();
  TEST("spline curve 2 creation");
  if ((curve2 = stp_curve_create_from_string(spline_curve_2)) == NULL)
    TEST_FAIL();
  else
    TEST_PASS();
  if (curve1)
    {
      if (verbose)
	stp_curve_write(stdout, curve1);
      TEST("spline curve 1 resample 1");
      if (stp_curve_resample(curve1, 41) == 0)
	TEST_FAIL();
      else
	{
	  TEST_PASS();
	  if (verbose)
	    stp_curve_write(stdout, curve1);
	}
      TEST("spline curve 1 resample 2");
      if (stp_curve_resample(curve1, 83) == 0)
	TEST_FAIL();
      else
	{
	  TEST_PASS();
	  if (verbose)
	    stp_curve_write(stdout, curve1);
	}
    }
  if (curve2)
    {
      if (verbose)
	stp_curve_write(stdout, curve2);
      TEST("spline curve 2 resample");
      if (stp_curve_resample(curve2, 48) == 0)
	TEST_FAIL();
      else
	{
	  TEST_PASS();
	  if (verbose)
	    stp_curve_write(stdout, curve2);
	}
    }
  TEST("compose add (PASS is an expected failure)");
  if (curve1 && curve2 &&
      stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, -1))
    {
      TEST_FAIL();
      if (!quiet)
	printf("compose with different wrap mode should fail!\n");
    }
  else
    TEST_PASS();

  TEST("linear curve 1 creation");
  if ((curve1 = stp_curve_create_from_string(linear_curve_1)) == NULL)
    TEST_FAIL();
  else
    TEST_PASS();
  TEST("linear curve 2 creation");
  if ((curve2 = stp_curve_create_from_string(linear_curve_2)) == NULL)
    TEST_FAIL();
  else
    TEST_PASS();
  if (curve1)
    {
      if (verbose)
	stp_curve_write(stdout, curve1);
      TEST("linear curve 1 resample");
      if (stp_curve_resample(curve1, 41) == 0)
	TEST_FAIL();
      else
	{
	  TEST_PASS();
	  if (verbose)
	    stp_curve_write(stdout, curve1);
	}
      stp_curve_free(curve1);
      curve1 = NULL;
    }
  if (curve2)
    {
      if (verbose)
	stp_curve_write(stdout, curve2);
      TEST("linear curve 2 resample");
      if (stp_curve_resample(curve2, 48) == 0)
	TEST_FAIL();
      else
	{
	  TEST_PASS();
	  if (verbose)
	    stp_curve_write(stdout, curve2);
	}
      stp_curve_free(curve2);
      curve2 = NULL;
    }

  curve1 = stp_curve_create(STP_CURVE_WRAP_AROUND);
  stp_curve_set_interpolation_type(curve1, STP_CURVE_TYPE_SPLINE);
  stp_curve_set_bounds(curve1, 0.0, 4.0);
  stp_curve_set_data(curve1, 48, standard_sat_adjustment);
  TEST("setting curve data");
  if (curve1 && (stp_curve_count_points(curve1) == 48))
    TEST_PASS();
  else
    TEST_FAIL();
  if (verbose)
    stp_curve_write(stdout, curve1);
  TEST("curve resample");
  if (stp_curve_resample(curve1, 384) == 0)
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve1);
    }
  if (curve1)
    {
      stp_curve_free(curve1);
      curve1 = NULL;
    }

  if (global_error_count)
    printf("%d/%d tests FAILED.\n", global_error_count, global_test_count);
  else
    printf("All tests passed successfully.\n");
  return global_error_count ? 1 : 0;
}
