/*
 * "$Id$"
 *
 *   Utility definitions for the CUPS driver development kit.
 *
 *   Copyright 1993-2000 by Easy Software Products.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636-3111 USA
 *
 *       Voice: (301) 373-9603
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 */

/*
 * Include necessary headers...
 */

#ifndef _UTIL_H_
#  define _UTIL_H_


/*
 * Macros...
 */

#define pwrite(s,n) fwrite((s), 1, (n), stdout)


/*
 * Dither control structures.
 */

typedef struct
{
  double value;
  unsigned bit_pattern;
  int is_dark;
  unsigned dot_size;
} simple_dither_range_t;

typedef struct
{
  double value;
  double lower;
  double upper;
  unsigned bit_pattern;
  int is_dark;
  unsigned dot_size;
} dither_range_t;

typedef struct
{
   double value_l;
   double value_h;
   unsigned bits_l;
   unsigned bits_h;
   int isdark_l;
   int isdark_h;
} full_dither_range_t;

/*
 * Prototypes...
 */

int	CheckBytes(const unsigned char *, int);
void	PackHorizontal(const unsigned char *, unsigned char *,
	               int, const unsigned char, const int);
void	PackHorizontal2(const unsigned char *, unsigned char *, int,
	        	const int);
void	PackHorizontalBit(const unsigned char *, unsigned char *,
	                  int, const unsigned char, const unsigned char);
void	PackVertical(const unsigned char *, unsigned char *,
	             int, const unsigned char, const int);


extern void *	init_dither(int in_width, int out_width, int horizontal_aspect,
			    int vertical_aspect, const char *dither_algorithm);
extern void	dither_set_transition(void *vd, double);
extern void	dither_set_density(void *vd, double);
extern void 	dither_set_black_lower(void *vd, double);
extern void 	dither_set_black_upper(void *vd, double);
extern void	dither_set_black_levels(void *vd, double, double, double);
extern void 	dither_set_randomizers(void *vd, double, double, double, double);
extern void 	dither_set_ink_darkness(void *vd, double, double, double);
extern void 	dither_set_light_inks(void *vd, double, double, double, double);
extern void	dither_set_c_ranges(void *vd, int nlevels,
				    const simple_dither_range_t *ranges,
				    double density);
extern void	dither_set_m_ranges(void *vd, int nlevels,
				    const simple_dither_range_t *ranges,
				    double density);
extern void	dither_set_y_ranges(void *vd, int nlevels,
				    const simple_dither_range_t *ranges,
				    double density);
extern void	dither_set_k_ranges(void *vd, int nlevels,
				    const simple_dither_range_t *ranges,
				    double density);
extern void	dither_set_k_ranges_full(void *vd, int nlevels,
					 const full_dither_range_t *ranges,
					 double density);
extern void	dither_set_c_ranges_full(void *vd, int nlevels,
					 const full_dither_range_t *ranges,
					 double density);
extern void	dither_set_m_ranges_full(void *vd, int nlevels,
					 const full_dither_range_t *ranges,
					 double density);
extern void	dither_set_y_ranges_full(void *vd, int nlevels,
					 const full_dither_range_t *ranges,
					 double density);
extern void	dither_set_c_ranges_simple(void *vd, int nlevels,
					   const double *levels, double density);
extern void	dither_set_m_ranges_simple(void *vd, int nlevels,
					   const double *levels, double density);
extern void	dither_set_y_ranges_simple(void *vd, int nlevels,
					   const double *levels, double density);
extern void	dither_set_k_ranges_simple(void *vd, int nlevels,
					   const double *levels, double density);
extern void	dither_set_c_ranges_complete(void *vd, int nlevels,
					     const dither_range_t *ranges);
extern void	dither_set_m_ranges_complete(void *vd, int nlevels,
					     const dither_range_t *ranges);
extern void	dither_set_y_ranges_complete(void *vd, int nlevels,
					     const dither_range_t *ranges);
extern void	dither_set_k_ranges_complete(void *vd, int nlevels,
					     const dither_range_t *ranges);
extern void	dither_set_ink_spread(void *vd, int spread);
extern void	dither_set_max_ink(void *vd, int, double);
extern void	dither_set_x_oversample(void *vd, int os);
extern void	dither_set_y_oversample(void *vd, int os);
extern void	dither_set_adaptive_divisor(void *vd, unsigned divisor);


extern void	free_dither(void *);

extern void	dither_fastblack(int, void *, unsigned char *);

extern void	dither_black(int, void *, unsigned char *);

extern void	dither_cmyk(int, void *, unsigned char *,
			    unsigned char *, unsigned char *,
			    unsigned char *, unsigned char *,
			    unsigned char *, unsigned char *);


#endif /* !_UTIL_H_ */.

/*
 * End of "$Id$".
 */
