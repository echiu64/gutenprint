/*
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*$Id$ */
/*                    */

#ifndef gdevstp_INCLUDED
#  define gdevstp_INCLUDED

/* Define the default X and Y resolution. */
#define X_DPI 360
#define Y_DPI 360

/*int write_bmp_header(P2(gx_device_printer *pdev, FILE *file));*/

/* 24-bit color mappers */
dev_proc_map_rgb_color(stp_map_16m_rgb_color);
dev_proc_map_color_rgb(stp_map_16m_color_rgb);

#endif				/* gdevstp_INCLUDED */
