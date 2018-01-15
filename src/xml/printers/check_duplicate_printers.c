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

#include <gutenprint/gutenprint.h>
#include <stdlib.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
  setenv("STP_CHECK_DUPLICATE_PRINTERS", "TRUE", 1);
  stp_init();			/* Aborts if duplicates are found */
  return 0;
}
