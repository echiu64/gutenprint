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

#define DEBUG_SIGNAL
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif

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
    "STP_CURVE;Wrap ;Linear ;48;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE; Wrap  ; Linear  ; 48 ; 0 ; 0 ; 4:0.5 ; 0.6 ; 0.7 ; 0.8 ; 0.9 ; 0.86 ; 0.82 ; 0.79 ; 0.78 ; 0.8 ; 0.83 ; 0.87 ; 0.9 ; 0.95 ; 1.05 ; 1.15 ; 1.3 ; 1.25 ; 1.2 ; 1.15 ; 1.12 ; 1.09 ; 1.06 ; 1.03 ; 1 ; 1 ; 1 ; 1 ; 1 ; 1 ; 1 ; 1 ; 1 ; 0.9 ; 0.8 ; 0.7 ; 0.65 ; 0.6 ; 0.55 ; 0.52 ; 0.48 ; 0.47 ; 0.47 ; 0.49 ; 0.49 ; 0.49 ; 0.52 ; 0.51 ; ",
    "STP_CURVE;Nowrap ;Linear ;48;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Nowrap ;Linear ;48;1.0;0;4:",
    "STP_CURVE;Nowrap ;Linear ;2;1.0;0;4:",
  };

static const int good_curve_count = sizeof(good_curves) / sizeof(const char *);

const char *bad_curves[] =
  {
    "STP_CURV;Wrap ;Linear ;48;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Warp ;Linear ;48;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Warp ;Lunatic ;48;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Warp ;LunaticLunaticLunaticLunaticLunatic ;48;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Wrap ;Linear ;-1;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Wrap ;Linear ;0;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Wrap ;Linear ;1;0;0;4:0.5;0.6;0.7;0.8;0.9;0.86;0.82;0.79;0.78;0.8;0.83;0.87;0.9;0.95;1.05;1.15;1.3;1.25;1.2;1.15;1.12;1.09;1.06;1.03;1;1;1;1;1;1;1;1;1;0.9;0.8;0.7;0.65;0.6;0.55;0.52;0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;",
    "STP_CURVE;Wrap ;Linear ;-1;1.0;0;4:",
    "STP_CURVE;Wrap ;Linear ;1;1.0;0;4:",
    "STP_CURVE;Wrap ;Linear ;48;1.0;0;4:",
    "STP_CURVE;Wrap ;Linear ;0;1.0;0;4:",
  };

const char *linear_curve_1 = "STP_CURVE;Nowrap ;Linear ;6;0;0;1:0;0;0;1;1;1;";
const char *linear_curve_2 = "STP_CURVE;Wrap ;Linear ;6;0;0;1:0;0;0;1;1;1;";
const char *spline_curve_1 = "STP_CURVE;Nowrap ;Spline ;6;0;0;1:0;0;0;1;1;1;";
const char *spline_curve_2 = "STP_CURVE;Wrap ;Spline ;6;0;0;1:0;0;0;1;1;1;";


static const int bad_curve_count = sizeof(bad_curves) / sizeof(const char *);

int global_error_count = 0;

int
main(int argc, char **argv)
{
  char *tmp1, tmp2;
  int i;
  stp_curve_t curve1 = stp_curve_allocate(STP_CURVE_WRAP_AROUND);
  stp_curve_t curve2 = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  stp_curve_t curve3;
  stp_curve_set_bounds(curve1, 0.0, 4.0);
  stp_curve_set_data(curve1, 48, standard_sat_adjustment);
  tmp1 = stp_curve_print_string(curve1);
  fprintf(stdout, "%s\n\n", tmp1);
  if (! stp_curve_read_string(tmp1, curve2))
    {
      fprintf(stderr, "stp_curve_read_string failed\n");
      global_error_count++;
    }
  free(tmp1);
  if (stp_curve_resample(curve2, 95) == 0)
    {
      fprintf(stderr, "stp_curve_resample failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve2);
  stp_curve_destroy(curve1);
  fprintf(stdout, "\n");
  for (i = 0; i < bad_curve_count; i++)
    {
      printf("SHOULD FAIL: %s\n", bad_curves[i]);
      if (stp_curve_read_string(bad_curves[i], curve2))
	{
	  printf("curve %d should have failed!\n", i);
	  global_error_count++;
	}
    }
  stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");
  for (i = 0; i < good_curve_count; i++)
    {
      printf("SHOULD PASS: %s\n", good_curves[i]);
      if (!stp_curve_read_string(good_curves[i], curve2))
	{
	  printf("curve %d should have passed!\n", i);
	  global_error_count++;
	  tmp1 = stp_curve_print_string(curve2);
	  if (strcmp(tmp1, good_curves[i]))
	    {
	      printf("curve read/write miscompare\n");
	      global_error_count++;
	    }
	  free(tmp1);
	}
    }
  stp_curve_destroy(curve2);
  printf("allocate 1\n");
  curve1 = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  printf("allocate 2\n");
  curve2 = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  printf("set gamma 1\n");
  if (!stp_curve_set_gamma(curve1, 1.2))
    {
      printf("set_gamma failed!\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  printf("set gamma 2\n");
  if (!stp_curve_set_gamma(curve2, -1.2))
    {
      printf("set_gamma failed!\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");
  printf("compose add\n");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_ADD, 64))
    {
      printf("add compose failed!\n");
      global_error_count++;
    }
  else
    stp_curve_print(stdout, curve3);
  fprintf(stdout, "\n");
  printf("resample 1\n");
  if (!stp_curve_resample(curve1, 64))
    {
      printf("resample failed!\n");
      global_error_count++;
    }
  else
    stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  stp_curve_destroy(curve3);
  printf("compose multiply\n");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, 64))
    {
      printf("multiply compose failed!\n");
      global_error_count++;
    }
  else
    stp_curve_print(stdout, curve3);
  fprintf(stdout, "\n");
  stp_curve_destroy(curve3);

  stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");

  if (!stp_curve_rescale(curve2, -1, STP_CURVE_COMPOSE_MULTIPLY,
			 STP_CURVE_BOUNDS_RESCALE))
    {
      printf("multiply rescale failed!\n");
      global_error_count++;
    }
  else
    stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_ADD, 64))
    {
      printf("subtract compose failed!\n");
      global_error_count++;
    }
  else
    stp_curve_print(stdout, curve3);
  fprintf(stdout, "\n");
  stp_curve_destroy(curve3);
  stp_curve_destroy(curve1);
  stp_curve_destroy(curve2);

  curve1 = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  curve2 = stp_curve_allocate(STP_CURVE_WRAP_AROUND);
  if (!stp_curve_read_string(spline_curve_1, curve1))
    {
      fprintf(stderr, "stp_curve_read_string failed\n");
      global_error_count++;
    }
  if (!stp_curve_read_string(spline_curve_2, curve2))
    {
      fprintf(stderr, "stp_curve_read_string failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  if (stp_curve_resample(curve1, 41) == 0)
    {
      fprintf(stderr, "stp_curve_resample failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  if (stp_curve_resample(curve1, 83) == 0)
    {
      fprintf(stderr, "stp_curve_resample failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");
  if (stp_curve_resample(curve2, 48) == 0)
    {
      fprintf(stderr, "stp_curve_resample failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");
  printf("compose add (should fail)\n");
  if (stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, -1))
    {
      printf("compose with different wrap mode should fail!\n");
      global_error_count++;
    }
  if (!stp_curve_read_string(linear_curve_1, curve1))
    {
      fprintf(stderr, "stp_curve_read_string failed\n");
      global_error_count++;
    }
  if (!stp_curve_read_string(linear_curve_2, curve2))
    {
      fprintf(stderr, "stp_curve_read_string failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  if (stp_curve_resample(curve1, 41) == 0)
    {
      fprintf(stderr, "stp_curve_resample failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");
  if (stp_curve_resample(curve2, 48) == 0)
    {
      fprintf(stderr, "stp_curve_resample failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve2);
  fprintf(stdout, "\n");
  stp_curve_destroy(curve1);
  stp_curve_destroy(curve2);

  curve1 = stp_curve_allocate(STP_CURVE_WRAP_AROUND);
  stp_curve_set_interpolation_type(curve1, STP_CURVE_TYPE_SPLINE);
  stp_curve_set_bounds(curve1, 0.0, 4.0);
  stp_curve_set_data(curve1, 48, standard_sat_adjustment);
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  if (stp_curve_resample(curve1, 384) == 0)
    {
      fprintf(stderr, "stp_curve_resample failed\n");
      global_error_count++;
    }
  stp_curve_print(stdout, curve1);
  fprintf(stdout, "\n");
  stp_curve_destroy(curve1);

  printf("%d total errors\n", global_error_count);
  return global_error_count ? 1 : 0;
}
