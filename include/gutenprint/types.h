/*
 *   Gutenprint type declarations
 *
 *   Copyright 2017 Robert Krawitz (rlk@alum.mit.edu)
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

/**
 * @file gutenprint/types.h
 * @brief Gutenprint dimension type declarations
 */

#ifndef GUTENPRINT_TYPES_H
#define GUTENPRINT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef double stp_dimension_t;
#define STP_DABS fabs
typedef int stp_resolution_t;
#define STP_RABS abs

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_TYPES_H */
