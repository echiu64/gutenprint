/*
 * "$Id$"
 *
 *   libgimpprint image functions.
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


#ifndef __GIMP_PRINT_IMAGE_H__
#define __GIMP_PRINT_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Constants...
 */

#define STP_CHANNEL_LIMIT	(32)

/*
 * Abstract data type for interfacing with the image creation program
 * (in this case, the Gimp).
 *
 * The progress_init(), note_progress(), and progress_conclude() members
 * are used to enable the image layer to deliver notification of
 * progress to the user.  It is likely that these functions will be
 * dropped in the future, and if desired must be implemented in
 * get_row().
 *
 * get_appname() should return the name of the application.  This is
 * embedded in the output by some drivers.
 *
 * width() and height() return the dimensions of the image in pixels.
 *
 * init() is used to perform any initialization required by the image
 * layer for the image.  It will be called once per image.  reset() is
 * called to reset the image to the beginning.  It may (in principle)
 * be called multiple times if a page is being printed more than once.
 * The reset() call may be removed in the future.
 *
 * get_appname() should return the name of the application.  This is
 * embedded in the output by some drivers.
 *
 * get_row() transfers the data from the image to the gimp-print
 * library.  It is called from the driver layer.  It should copy WIDTH
 * (as returned by the width() member) pixels of data into the data
 * buffer.  It normally returns STP_IMAGE_STATUS_OK; if something goes wrong,
 * or the application wishes to stop producing any further output
 * (e. g. because the user cancelled the print job), it should return
 * STP_IMAGE_STATUS_ABORT.  This will cause the driver to flush any remaining
 * data to the output.  It will always request rows in monotonically
 * ascending order, but it may skip rows (if, for example, the
 * resolution of the input is higher than the resolution of the
 * output).
 */


typedef enum
{
  STP_IMAGE_STATUS_OK,
  STP_IMAGE_STATUS_ABORT
} stp_image_status_t;


typedef struct stp_image
{
  void (*init)(struct stp_image *image);
  void (*reset)(struct stp_image *image);
  int  (*width)(struct stp_image *image);
  int  (*height)(struct stp_image *image);
  stp_image_status_t (*get_row)(struct stp_image *image, unsigned char *data,
                                size_t byte_limit, int row);
  const char *(*get_appname)(struct stp_image *image);
  void (*conclude)(struct stp_image *image);
  void *rep;
} stp_image_t;


#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_IMAGE_H__ */
/*
 * End of "$Id$".
 */
