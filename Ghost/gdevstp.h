
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
