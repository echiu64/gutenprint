/*
 * Check for duplicate printers (at build time).
 *
 *   Copyright 2018 Robert Krawitz (rlk@alum.mit.edu)
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gutenprint/gutenprint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

int
main(int argc, char **argv)
{
  if (getenv("STP_TEST_LOG_PREFIX"))
    {
      char path[PATH_MAX+1];
      if (getenv("BUILD_VERBOSE"))
	dup2(2, 3);
      (void) snprintf(path, PATH_MAX, "%scheck_duplicate_printers_%d.log", getenv("STP_TEST_LOG_PREFIX"), getpid());
      freopen(path, "w", stdout);
      dup2(1, 2);
    }

#if defined(HAVE_SETENV)
  setenv("STP_CHECK_DUPLICATE_PRINTERS", "TRUE", 1);
#else
  putenv("STP_CHECK_DUPLICATE_PRINTERS=TRUE");
#endif

  fprintf(stderr, "CHECK_DUPLICATE_PRINTERS\n");
  stp_init();			/* Aborts if duplicates are found */
  return 0;
}
