/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 2003 Robert Krawitz (rlk@alum.mit.edu)
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


#ifndef GIMP_PRINT_INTERNAL_IMAGE_H
#define GIMP_PRINT_INTERNAL_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern void stpi_image_init(stp_image_t *image);
extern void stpi_image_reset(stp_image_t *image);
extern int stpi_image_bpp(stp_image_t *image);
extern int stpi_image_width(stp_image_t *image);
extern int stpi_image_height(stp_image_t *image);
extern stp_image_status_t stpi_image_get_row(stp_image_t *image,
					     unsigned char *data,
					     size_t limit, int row);
extern const char *stpi_image_get_appname(stp_image_t *image);
extern void stpi_image_progress_init(stp_image_t *image);
extern void stpi_image_note_progress(stp_image_t *image,
				     double current, double total);
extern void stpi_image_progress_conclude(stp_image_t *image);
extern void stpi_image_transpose(stp_image_t *image);
extern void stpi_image_hflip(stp_image_t *image);
extern void stpi_image_vflip(stp_image_t *image);
extern void stpi_image_rotate_ccw(stp_image_t *image);
extern void stpi_image_rotate_cw(stp_image_t *image);
extern void stpi_image_rotate_180(stp_image_t *image);
extern void stpi_image_crop(stp_image_t *image, int left, int top,
			    int right, int bottom);

#endif /* GIMP_PRINT_INTERNAL_IMAGE_H */
/*
 * End of "$Id$".
 */
