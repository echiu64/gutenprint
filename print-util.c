/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
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
 *
 * Contents:
 *
 *   dither_black()       - Dither grayscale pixels to black.
 *   dither_cmyk()        - Dither RGB pixels to cyan, magenta, yellow, and
 *                          black.
 *   dither_black4()      - Dither grayscale pixels to 4 levels of black.
 *   dither_cmyk4()       - Dither RGB pixels to 4 levels of cyan, magenta,
 *                          yellow, and black.
 *   gray_to_gray()       - Convert grayscale image data to grayscale.
 *   indexed_to_gray()    - Convert indexed image data to grayscale.
 *   indexed_to_rgb()     - Convert indexed image data to RGB.
 *   rgb_to_gray()        - Convert RGB image data to grayscale.
 *   rgb_to_rgb()         - Convert RGB image data to RGB.
 *   default_media_size() - Return the size of a default page size.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.64  2000/02/05 20:02:10  rlk
 *   some more silly problems
 *
 *   Revision 1.63  2000/02/05 14:56:41  rlk
 *   1) print-util.c: decrement rather than increment counter!
 *
 *   2) print-escp2.c: don't advance the paper a negative (or, with some printers,
 *   a very large positive) amount.
 *
 *   Revision 1.62  2000/02/04 09:40:28  gandy
 *   Models BJC-1000/2000/3000/6000/6100/7000/7100 ready for testing.
 *
 *   Revision 1.61  2000/02/04 01:02:15  rlk
 *   Prelim support for 850/860/870/1200; fix stupid bug in ESC(S
 *
 *   Revision 1.60  2000/02/02 13:17:10  rlk
 *   Add a few more parameters to the dither_t struct.
 *
 *   Revision 1.59  2000/02/02 03:03:55  rlk
 *   Move all the constants into members of a struct.  This will eventually permit
 *   us to use different dithering constants for each printer, or even vary them
 *   on the fly.  Currently there's a static dither_t that contains constants,
 *   but that's the easy part to fix...
 *
 *   Revision 1.58  2000/02/01 09:01:40  gandy
 *   Add print-canon.c: Support for the BJC 6000 and possibly others
 *
 *   Revision 1.57  2000/01/29 02:34:30  rlk
 *   1) Remove globals from everything except print.c.
 *
 *   2) Remove broken 1440x720 and 2880x720 microweave modes.
 *
 *   Revision 1.56  2000/01/28 03:59:53  rlk
 *   Move printers to print-util; also add top/left/bottom/right boxes to the UI
 *
 *   Revision 1.55  2000/01/25 19:51:27  rlk
 *   1) Better attempt at supporting newer Epson printers.
 *
 *   2) Generalized paper size support.
 *
 *   Revision 1.54  2000/01/21 00:53:39  rlk
 *   1) Add a few more paper sizes.
 *
 *   2) Clean up Makefile.standalone.
 *
 *   3) Nominal support for Stylus Color 850.
 *
 *   Revision 1.53  2000/01/21 00:18:59  rlk
 *   Describe the algorithms in print-util.c.
 *
 *   Revision 1.52  2000/01/17 22:23:31  rlk
 *   Print 3.1.0
 *
 *   Revision 1.51  2000/01/17 02:05:47  rlk
 *   Much stuff:
 *
 *   1) Fixes from 3.0.5
 *
 *   2) First cut at enhancing monochrome and four-level printing with stuff from
 *   the color print function.
 *
 *   3) Preliminary support (pre-support) for 440/640/740/900/750/1200.
 *
 *   Revision 1.50  2000/01/15 00:57:53  rlk
 *   Intermediate version
 *
 *   Revision 1.49  2000/01/08 23:30:37  rlk
 *   Some tweaking
 *
 *   Revision 1.48  1999/12/30 23:58:07  rlk
 *   Silly little bug...
 *
 *   Revision 1.47  1999/12/26 19:02:46  rlk
 *   Performance stuff
 *
 *   Revision 1.46  1999/12/25 17:47:17  rlk
 *   Cleanup
 *
 *   Revision 1.45  1999/12/25 00:41:01  rlk
 *   some minor improvement
 *
 *   Revision 1.44  1999/12/24 12:57:38  rlk
 *   Reduce grain; improve red
 *
 *   Revision 1.43  1999/12/22 03:24:34  rlk
 *   round length up, not down
 *
 *   Revision 1.42  1999/12/22 03:12:17  rlk
 *   More constant fiddling
 *
 *   Revision 1.41  1999/12/22 01:34:28  rlk
 *   Reverse direction each pass
 *
 *   Revision 1.40  1999/12/18 23:45:07  rlk
 *   typo
 *
 *   Revision 1.39  1999/12/12 20:49:01  rlk
 *   Various changes
 *
 *   Revision 1.38  1999/12/11 23:12:06  rlk
 *   Better matching between cmy/k
 *
 *   Smoother dither!
 *
 *   Revision 1.37  1999/12/05 23:24:08  rlk
 *   don't want PRINT_LUT in release
 *
 *   Revision 1.36  1999/12/05 04:33:34  rlk
 *   Good results for the night.
 *
 *   Revision 1.35  1999/12/04 19:01:05  rlk
 *   better use of light colors
 *
 *   Revision 1.34  1999/12/02 02:09:45  rlk
 *   .
 *
 *   Revision 1.33  1999/11/25 00:02:03  rlk
 *   Revamped many controls
 *
 *   Revision 1.32  1999/11/23 02:11:37  rlk
 *   Rationalize variables, pass 3
 *
 *   Revision 1.31  1999/11/23 01:33:37  rlk
 *   First stage of simplifying the variable stuff
 *
 *   Revision 1.30  1999/11/16 00:59:00  rlk
 *   More fine tuning
 *
 *   Revision 1.29  1999/11/14 21:37:13  rlk
 *   Revamped contrast
 *
 *   Revision 1.28  1999/11/14 18:59:22  rlk
 *   Final preparations for release to Olof
 *
 *   Revision 1.27  1999/11/14 00:57:11  rlk
 *   Mix black in sooner gives better density.
 *
 *   Revision 1.26  1999/11/13 02:31:29  rlk
 *   Finally!  Good settings!
 *
 *   Revision 1.25  1999/11/12 03:34:40  rlk
 *   More tweaking
 *
 *   Revision 1.24  1999/11/12 02:18:32  rlk
 *   Stubs for dynamic memory allocation
 *
 *   Revision 1.23  1999/11/12 01:53:37  rlk
 *   Remove silly spurious stuff
 *
 *   Revision 1.22  1999/11/12 01:51:47  rlk
 *   Much better black
 *
 *   Revision 1.21  1999/11/10 01:13:06  rlk
 *   Support up to 2880 dpi
 *
 *   Revision 1.20  1999/11/07 22:16:42  rlk
 *   Bug fixes; try to improve dithering slightly
 *
 *   Revision 1.19  1999/10/29 01:01:16  rlk
 *   Smoother rendering of darker colors
 *
 *   Revision 1.18  1999/10/28 02:01:15  rlk
 *   One bug, two effects:
 *
 *   1) Handle 4-color correctly (it was treating the 4-color too much like the
 *   6-color).
 *
 *   2) An attempt to handle both cases with the same code path led to a
 *   discontinuity that depending upon the orientation of a color gradient would
 *   lead to either white or dark lines at the point that the dark version of
 *   the color would kick in.
 *
 *   Revision 1.17  1999/10/26 23:58:31  rlk
 *   indentation
 *
 *   Revision 1.16  1999/10/26 23:36:51  rlk
 *   Comment out all remaining 16-bit code, and rename 16-bit functions to "standard" names
 *
 *   Revision 1.15  1999/10/26 02:10:30  rlk
 *   Mostly fix save/load
 *
 *   Move all gimp, glib, gtk stuff into print.c (take it out of everything else).
 *   This should help port it to more general purposes later.
 *
 *   Revision 1.14  1999/10/25 23:31:59  rlk
 *   16-bit clean
 *
 *   Revision 1.13  1999/10/25 00:14:46  rlk
 *   Remove more of the 8-bit code, now that it is tested
 *
 *   Revision 1.12  1999/10/23 20:26:48  rlk
 *   Move LUT calculation to print-util
 *
 *   Revision 1.11  1999/10/21 01:27:37  rlk
 *   More progress toward full 16-bit rendering
 *
 *   Revision 1.10  1999/10/19 02:04:59  rlk
 *   Merge all of the single-level print_cmyk functions
 *
 *   Revision 1.9  1999/10/18 01:37:02  rlk
 *   Remove spurious stuff
 *
 *   Revision 1.8  1999/10/17 23:44:07  rlk
 *   16-bit everything (untested)
 *
 *   Revision 1.7  1999/10/17 23:01:01  rlk
 *   Move various dither functions into print-utils.c
 *
 *   Revision 1.6  1999/10/14 01:59:59  rlk
 *   Saturation
 *
 *   Revision 1.5  1999/10/03 23:57:20  rlk
 *   Various improvements
 *
 *   Revision 1.4  1999/09/18 15:18:47  rlk
 *   A bit more random
 *
 *   Revision 1.3  1999/09/14 21:43:43  rlk
 *   Some hoped-for improvements
 *
 *   Revision 1.2  1999/09/12 00:12:24  rlk
 *   Current best stuff
 *
 *   Revision 1.10  1998/05/17 07:16:49  yosh
 *   0.99.31 fun
 *
 *   updated print plugin
 *
 *   -Yosh
 *
 *   Revision 1.14  1998/05/16  18:27:59  mike
 *   Cleaned up dithering functions - unnecessary extra data in dither buffer.
 *
 *   Revision 1.13  1998/05/13  17:00:36  mike
 *   Minor change to CMYK generation code - now cube black difference value
 *   for better colors.
 *
 *   Revision 1.12  1998/05/08  19:20:50  mike
 *   Updated CMYK generation code to use new method.
 *   Updated dithering algorithm (slightly more uniform now, less speckling)
 *   Added default media size function.
 *
 *   Revision 1.11  1998/03/01  18:03:27  mike
 *   Whoops - need to add 255 - alpha to the output values (transparent to white
 *   and not transparent to black...)
 *
 *   Revision 1.10  1998/03/01  17:20:48  mike
 *   Updated alpha code to do alpha computation before gamma/brightness lut.
 *
 *   Revision 1.9  1998/03/01  17:13:46  mike
 *   Updated CMY/CMYK conversion code for dynamic BG and hue adjustment.
 *   Added alpha channel support to color conversion functions.
 *
 *   Revision 1.8  1998/01/21  21:33:47  mike
 *   Replaced Burkes dither with stochastic (random) dither.
 *
 *   Revision 1.7  1997/10/02  17:57:26  mike
 *   Replaced ordered dither with Burkes dither (error-diffusion).
 *   Now dither K separate from CMY.
 *
 *   Revision 1.6  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.5  1997/07/26  18:43:04  mike
 *   Fixed dither_black and dither_cmyk - wasn't clearing extra bits
 *   (caused problems with A3/A4 size output).
 *
 *   Revision 1.5  1997/07/26  18:43:04  mike
 *   Fixed dither_black and dither_cmyk - wasn't clearing extra bits
 *   (caused problems with A3/A4 size output).
 *
 *   Revision 1.4  1997/07/02  18:46:26  mike
 *   Fixed stupid bug in dither_black() - wasn't comparing against gray
 *   pixels (comparing against the first black byte - d'oh!)
 *   Changed 255 in dither matrix to 254 to shade correctly.
 *
 *   Revision 1.4  1997/07/02  18:46:26  mike
 *   Fixed stupid bug in dither_black() - wasn't comparing against gray
 *   pixels (comparing against the first black byte - d'oh!)
 *   Changed 255 in dither matrix to 254 to shade correctly.
 *
 *   Revision 1.3  1997/07/02  13:51:53  mike
 *   Added rgb_to_rgb and gray_to_gray conversion functions.
 *   Standardized calling args to conversion functions.
 *
 *   Revision 1.2  1997/07/01  19:28:44  mike
 *   Updated dither matrix.
 *   Fixed scaling bugs in dither_*() functions.
 *
 *   Revision 1.1  1997/06/19  02:18:15  mike
 *   Initial revision
 */

/* #define PRINT_DEBUG */


#include "print.h"
#include <math.h>

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8


/*
 * Error buffer for dither functions.  This needs to be at least 14xMAXDPI
 * (currently 720) to avoid problems...
 *
 * Want to dynamically allocate this so we can save memory!
 */

#define ERROR_ROWS 2
#define NCOLORS (4)

typedef union error
{
  struct
  {
    int c[ERROR_ROWS];
    int m[ERROR_ROWS];
    int y[ERROR_ROWS];
    int k[ERROR_ROWS];
  } c;
  int v[4][ERROR_ROWS];
} error_t;

error_t *nerror = 0;

static int error[ERROR_ROWS][NCOLORS][MAX_CARRIAGE_WIDTH*MAX_BPI+1];

/*
 * Dithering functions!
 *
 * We currently have four dithering functions:
 *
 * 1) dither_black produces a single level of black from grayscale input.
 *    This is used by most printers when printing grayscale.
 *
 * 2) dither_cmyk produces 3, 4, 6, or 7 color output (actually, it can
 *    deal with any combination of dark and light colored inks, but not a
 *    "light black" ink, although that would be nice :-) ).
 *
 * 3) dither_black4 produces four levels of black (corresponding to three
 *    dot sizes and no ink).  This was originally written by Mike Sweet
 *    for various HP printers, but it's useful for some of the newer Epson
 *    Stylus printers, too.  THIS HAS NOT BEEN TESTED IN 3.1.
 *
 * 4) dither_cmyk4 does likewise for color output.  THIS HAS NOT BEEN TESTED
 *    IN 3.1.
 *
 * Many of these routines (in particular dither_cmyk and the 4-level functions)
 * have constants hard coded that are tuned for particular printers.  Needless
 * to say, this must go.
 *
 * The dithering algorithm is a basic error diffusion, with a few tweaks of
 * my own.  Error diffusion works by taking the output error at a given pixel
 * and "diffusing" it into surrounding pixels.  Output error is the difference
 * between the amount of ink output and the input level at each pixel.  For
 * simple printers, with one or four ink colors and only one dot size, the
 * amount of ink output is either 65536 (i. e. full output) or 0 (no output).
 * The difference between this and the input level is the error.  Normal
 * error diffusion adds part of this error to the adjoining pixels in the
 * next column and the next row (the algorithm simply scans each row in turn,
 * never backing up).  The error adds up until it reaches a threshold (half of
 * the full output level, or 32768), at which point a dot is output, the
 * output is subtracted from the current value, and the (now negative) error
 * is diffused similarly.
 *
 * Handling multiple output levels makes life a bit more complicated.  In
 * principle, it shouldn't be much harder: simply figure out what the ratio
 * between the available output levels is and have multiple thresholds.
 * In practice, getting these right involves a lot of trial and error.  The
 * other thing that's important is to maximize the number of dots that have
 * some ink.  This will reduce the amount of speckling.  More on this later.
 *
 * The next question: how do we handle black when printing in color?  Black
 * ink is much darker than colored inks.  It's possible to produce black by
 * adding some mixture of cyan, magenta, and yellow -- in principle.  In
 * practice, the black really isn't very black, and different inks and
 * different papers will produce different color casts.  However, by using
 * CMY to produce gray, we can output a lot more dots!  This makes for a much
 * smoother image.  What's more, one cyan, one magenta, and one yellow dot
 * produce less darkness than one black dot, so we're outputting that many
 * more dots.  Better yet, with 6 or 7 color printers, we have to output even
 * more light ink dots.  So Epson Stylus Photo printers can produce really
 * smooth grays -- if we do everything right.  The right idea is to use
 * CMY at lower black levels, and gradually mix in black as the overall
 * amount of ink increases, so the black dots don't really become visible
 * within the ink mass.
 *
 * I stated earlier that I've tweaked the basic error diffusion algorithm.
 * Here's what I've done to improve it:
 *
 * 1) We use a randomized threshold to decide when to print.  This does
 *    two things for us: it reduces the slightly squiggly diagonal lines
 *    that are the mark of error diffusion; and it allows us to lay down
 *    some ink even in very light areas near the edge of the image.
 *    The squiggly lines that error diffusion algorithms tend to generate
 *    are caused by the gradual accumulation of error.  This error is
 *    partially added horizontally and partially vertically.  The horizontal
 *    accumulation results in a dot eventually being printed.  The vertical
 *    accumulation results in a dot getting laid down in roughly the same
 *    horizontal position in the next row.  The diagonal squigglies result
 *    from the error being added to pixels one forward and one below the
 *    current pixel; these lines slope from the top right to the bottom left
 *    of the image.
 *
 *    Error diffusion also results in pale areas being completely white near
 *    the top left of the image (the origin of the printing coordinates).
 *    This is because enough error has to accumulate for anything at all to
 *    get printed.
 *
 *    Randomizing the threshold somewhat breaks up the diagonals to some
 *    degree by randomizing the exact location that the accumulated output
 *    crosses the threshold.  It reduces the false white areas by allowing
 *    some dots to be printed even when the accumulated output level is very
 *    low.  It doesn't result in excess ink because the full output level
 *    is still subtracted and diffused.
 *
 * 2) Alternating scan direction between rows (first row is scanned left to
 *    right, second is scanned right to left, and so on).  This also helps
 *    break up white areas, and it also seems to break up squigglies a bit.
 *    Furthermore, it eliminates directional biases in the horizontal
 *    direction.
 *
 * 3) Diffusing the error into more pixels.  Instead of diffusing the entire
 *    error into (X+1, Y) and (X, Y+1), we diffuse it into (X+1, Y),
 *    (X+K, Y+1), (X, Y+1), (X-K, Y+1) where K depends upon the output level
 *    (it never exceeds about 5 dots, and is greater at higher output
 *    levels).  This really reduces squigglies and graininess.
 *
 * 4) Don't lay down any colored ink if we're laying down black ink.  There's
 *    no point; the colored ink won't show.  We still pretend that we did
 *    for purposes of error diffusion (otherwise excessive error will build
 *    up, and will take a long time to clear, resulting in heavy bleeding of
 *    ink into surrounding areas, which is very ugly indeed), but we don't
 *    bother wasting the ink.
 *
 * 5) Oversampling.  This is how to print 1440x720 with Epson Stylus printers.
 *    Printing full density at 1440x720 will result in excess ink being laid
 *    down.  The trick is to print only every other dot.  We still compute
 *    the error as though we printed every dot.  It turns out that
 *    randomizing which dots are printed results in very speckled output.
 *
 * What about multiple output levels?  For 6 and 7 color printers, simply
 * using different threshold levels has a problem: the pale inks have trouble
 * being seen when a lot of darker ink is being printed.  So rather than
 * just using the output level of the particular color to decide which ink
 * to print, we look at the total density (sum of all output levels).
 * If the density's high enough, we prefer to use the dark ink.  Speckling
 * is less visible when there's a lot of ink, anyway.  I haven't yet figured
 * out what to do for multiple levels of one color.
 *
 * Speaking of 6 colors a bit more, I also determined (empirically!) that
 * simply subtracting the appropriate output level (full for the dark ink,
 * partial for the light ink) results in speckling.  Why?  Well, after printing
 * a dark dot, it takes longer to "recharge" the output level than after
 * printing a light dot.  What I do instead is compute a probability of
 * printing either a light or a dark dot when I exceed the threshold.  Picking
 * a random number decides which color ink to lay down.  However, rather than
 * subtracting the level appropriate for what I printed, I subtract a scaled
 * amount between the two levels.  The amount is scaled by the probability:
 * (L + P * (D - L)) where L is the light level, P is the probability of
 * printing dark, D is the dark level.  This also results in smoother output.
 *
 * You'll note that I haven't quoted a single source on color or printing
 * theory.  I simply did all of this empirically.
 *
 * There are various other tricks to reduce speckling.  One that I've seen
 * is to reduce the amount of ink printed in regions where one color
 * (particularly cyan, which is perceived as the darkest) is very pale.
 * This does reduce speckling all right, but it also results in strange
 * tonal curves and weird (to my eye) colors.
 *
 * Another, better trick is to print fixed patterns corresponding to given
 * output levels (basically a patterned screen).  This is probably much
 * better, but it's harder to get right in areas of rapidly varying color
 * and probably requires more lookup tables.  But it might be worth a shot.
 */

/*
 * 'dither_black()' - Dither grayscale pixels to black.
 */

static dither_t dither_info;

void
init_dither(void)
{
  (void) memset(error, 0, sizeof(error));
  dither_info.cbits = 1;
  dither_info.lcbits = 1;
  dither_info.mbits = 1;
  dither_info.lmbits = 1;
  dither_info.ybits = 1;
  dither_info.lybits = 1;
  dither_info.kbits = 1;
  dither_info.k_lower = 12 * 256;
  dither_info.k_upper = 128 * 256;
  dither_info.lc_level = 32768;
  dither_info.lm_level = 32768;
  dither_info.ly_level = 32768;
  dither_info.c_randomizer = 0;
  dither_info.m_randomizer = 0;
  dither_info.y_randomizer = 0;
  dither_info.k_randomizer = 4;
  dither_info.k_clevel = 32;
  dither_info.k_mlevel = 32;
  dither_info.k_ylevel = 32;
  dither_info.c_darkness = 22;
  dither_info.m_darkness = 16;
  dither_info.y_darkness = 10;
  dither_info.nc_l = 4;
  dither_info.c_transitions = malloc(4 * sizeof(int));
  dither_info.c_levels = malloc(4 * sizeof(int));
  dither_info.nlc_l = 4;
  dither_info.lc_transitions = malloc(4 * sizeof(int));
  dither_info.lc_levels = malloc(4 * sizeof(int));
  dither_info.nm_l = 4;
  dither_info.m_transitions = malloc(4 * sizeof(int));
  dither_info.m_levels = malloc(4 * sizeof(int));
  dither_info.nlm_l = 4;
  dither_info.lm_transitions = malloc(4 * sizeof(int));
  dither_info.lm_levels = malloc(4 * sizeof(int));
  dither_info.ny_l = 4;
  dither_info.y_transitions = malloc(4 * sizeof(int));
  dither_info.y_levels = malloc(4 * sizeof(int));
  dither_info.nly_l = 4;
  dither_info.ly_transitions = malloc(4 * sizeof(int));
  dither_info.ly_levels = malloc(4 * sizeof(int));
  dither_info.nk_l = 4;
  dither_info.k_transitions = malloc(4 * sizeof(int));
  dither_info.k_levels = malloc(4 * sizeof(int));
  dither_info.c_levels[0] = 0;
  dither_info.c_transitions[0] = 0;
  dither_info.c_levels[1] = 32767;
  dither_info.c_transitions[1] = (32767 + 0) / 2;
  dither_info.c_levels[2] = 213 * 256;
  dither_info.c_transitions[2] = ((213 * 256) + 32767) / 2;
  dither_info.c_levels[3] = 65535;
  dither_info.c_transitions[3] = (65535 + (213 * 256)) / 2;
  dither_info.lc_levels[0] = 0;
  dither_info.lc_transitions[0] = 0;
  dither_info.lc_levels[1] = 32767;
  dither_info.lc_transitions[1] = (32767 + 0) / 2;
  dither_info.lc_levels[2] = 213 * 256;
  dither_info.lc_transitions[2] = ((213 * 256) + 32767) / 2;
  dither_info.lc_levels[3] = 65535;
  dither_info.lc_transitions[3] = (65535 + (213 * 256)) / 2;
  dither_info.m_levels[0] = 0;
  dither_info.m_transitions[0] = 0;
  dither_info.m_levels[1] = 32767;
  dither_info.m_transitions[1] = (32767 + 0) / 2;
  dither_info.m_levels[2] = 213 * 256;
  dither_info.m_transitions[2] = ((213 * 256) + 32767) / 2;
  dither_info.m_levels[3] = 65535;
  dither_info.m_transitions[3] = (65535 + (213 * 256)) / 2;
  dither_info.lm_levels[0] = 0;
  dither_info.lm_transitions[0] = 0;
  dither_info.lm_levels[1] = 32767;
  dither_info.lm_transitions[1] = (32767 + 0) / 2;
  dither_info.lm_levels[2] = 213 * 256;
  dither_info.lm_transitions[2] = ((213 * 256) + 32767) / 2;
  dither_info.lm_levels[3] = 65535;
  dither_info.lm_transitions[3] = (65535 + (213 * 256)) / 2;
  dither_info.y_levels[0] = 0;
  dither_info.y_transitions[0] = 0;
  dither_info.y_levels[1] = 32767;
  dither_info.y_transitions[1] = (32767 + 0) / 2;
  dither_info.y_levels[2] = 213 * 256;
  dither_info.y_transitions[2] = ((213 * 256) + 32767) / 2;
  dither_info.y_levels[3] = 65535;
  dither_info.y_transitions[3] = (65535 + (213 * 256)) / 2;
  dither_info.ly_levels[0] = 0;
  dither_info.ly_transitions[0] = 0;
  dither_info.ly_levels[1] = 32767;
  dither_info.ly_transitions[1] = (32767 + 0) / 2;
  dither_info.ly_levels[2] = 213 * 256;
  dither_info.ly_transitions[2] = ((213 * 256) + 32767) / 2;
  dither_info.ly_levels[3] = 65535;
  dither_info.ly_transitions[3] = (65535 + (213 * 256)) / 2;
  dither_info.k_levels[0] = 0;
  dither_info.k_transitions[0] = 0;
  dither_info.k_levels[1] = 32767;
  dither_info.k_transitions[1] = (32767 + 0) / 2;
  dither_info.k_levels[2] = 213 * 256;
  dither_info.k_transitions[2] = ((213 * 256) + 32767) / 2;
  dither_info.k_levels[3] = 65535;
  dither_info.k_transitions[3] = (65535 + (213 * 256)) / 2;
}  

void
free_dither()
{
  free(dither_info.c_transitions);
  free(dither_info.c_levels);
  free(dither_info.lc_transitions);
  free(dither_info.lc_levels);
  free(dither_info.m_transitions);
  free(dither_info.m_levels);
  free(dither_info.lm_transitions);
  free(dither_info.lm_levels);
  free(dither_info.y_transitions);
  free(dither_info.y_levels);
  free(dither_info.ly_transitions);
  free(dither_info.ly_levels);
  free(dither_info.k_transitions);
  free(dither_info.k_levels);
}

void
dither_black(unsigned short     *gray,		/* I - Grayscale pixels */
	     int           	row,		/* I - Current Y coordinate */
	     int           	src_width,	/* I - Width of input row */
	     int           	dst_width,	/* I - Width of output row */
	     unsigned char 	*black,		/* O - Black bitmap pixels */
	     int horizontal_overdensity)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dithering bitmask */
  dither_t     *d = &dither_info;
  int overdensity_bits = 0;
  int terminate;
  int direction = row & 1 ? 1 : -1;

  switch (horizontal_overdensity)
    {
    case 0:
    case 1:
      overdensity_bits = 0;
      break;
    case 2:
      overdensity_bits = 1;
      break;
    case 4:
      overdensity_bits = 2;
      break;
    case 8:
      overdensity_bits = 3;
      break;
    }

  bit = (direction == 1) ? 128 : 1 << (7 - ((dst_width - 1) & 7));
  x = (direction == 1) ? 0 : dst_width - 1;
  terminate = (direction == 1) ? dst_width : -1;

  xstep  = src_width / dst_width;
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];
  memset(kerror1, 0, dst_width * sizeof(int));

  memset(black, 0, length);
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      kerror0 += dst_width - 1;
      kerror1 += dst_width - 1;
      kptr = black + length - 1;
      xstep = -xstep; 
      gray += src_width - 1;
      xerror = ((dst_width - 1) * xmod) % dst_width;
      xmod = -xmod;
    }

  for (ditherbit = rand() & 0xffff, ditherk = kerror0[0];
       x != terminate;
       ditherbit = rand() & 0xffff,
       x += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {
    int offset;
    k = 65535 - *gray + ditherk / 8;
    offset = (15 - (((k & 0xf000) >> 12)) * horizontal_overdensity) >> 1;
    if (x < offset)
      offset = x;
    else if (x > dst_width - offset - 1)
      offset = dst_width - x - 1;

    if (k > ditherbit)
    {
      if (d->kbits++ == horizontal_overdensity)
	{
	  *kptr |= bit;
	  d->kbits = 1;
	}
      k -= 65535;
    }

    if (ditherbit & bit)
    {
      kerror1[-offset] += k;
      kerror1[0] += 3 * k;
      kerror1[0] += k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[-offset] += k;
      kerror1[0] += k;
      kerror1[0] += k;
      ditherk    = kerror0[1] + 5 * k;
    }

    if (direction == 1)
      {
	if (bit == 1)
	  {
	    kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    gray   += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
      {
	xerror -= dst_width;
	gray   += direction;
      }
    else if (xerror < 0)
      {
	xerror += dst_width;
	gray   += direction;
      }      
  }
}

/*
 * 'dither_cmyk6()' - Dither RGB pixels to cyan, magenta, light cyan,
 * light magenta, yellow, and black.
 *
 * Added by Robert Krawitz <rlk@alum.mit.edu> August 30, 1999.
 *
 * Let's be really aggressive and use a single routine for ALL cmyk dithering,
 * including 6 and 7 color.
 *
 * Note that this is heavily tuned for Epson Stylus Photo printers.
 * This should be generalized for other CMYK and CcMmYK printers.  All
 * of these constants were empirically determined, and are subject to review.
 */

/*
 * Ratios of dark to light inks.  The darker ink should be DE / NU darker
 * than the light ink.
 *
 * It is essential to be very careful about use of parentheses with these
 * macros!
 *
 * Increasing the denominators results in use of more dark ink.  This creates
 * more saturated colors.
 */

#define NU_C 1
#define DE_C 1
#define NU_M 1
#define DE_M 1
#define NU_Y 1
#define DE_Y 1

#define I_RATIO_C1 NU_C / (DE_C + NU_C)
const static int C_CONST = 65536 * I_RATIO_C1;

#define I_RATIO_M1 NU_M / (DE_M + NU_M)
const static int M_CONST = 65536 * I_RATIO_M1;

#define I_RATIO_Y1 NU_Y / (DE_Y + NU_Y)
const static int Y_CONST = 65536 * I_RATIO_Y1;

/*
 * Lower and upper bounds for mixing CMY with K to produce gray scale.
 * Reducing KDARKNESS_LOWER results in more black being used with relatively
 * light grays, which causes speckling.  Increasing KDARKNESS_UPPER results
 * in more CMY being used in dark tones, which results in less pure black.
 * Decreasing the gap too much results in sharp crossover and stairstepping.
 */

/*
 * Randomizing values for deciding when to output a bit.  Normally with the
 * error diffusion algorithm a bit is not output until the accumulated value
 * of the pixel crosses a threshold.  This randomizes the threshold, which
 * results in fewer obnoxious diagonal jaggies in pale regions.  Smaller values
 * result in greater randomizing.  We use less randomness for black output
 * to avoid production of black speckles in light regions.
 */

#define UPDATE_COLOR(r)				\
do {						\
  o##r = r;					\
  r += dither##r / 8;				\
} while (0)

#define DO_PRINT_COLOR(color)				\
do {							\
  if (d->color##bits++ == horizontal_overdensity)	\
    {							\
      *color##ptr |= bit;				\
      d->color##bits = 1;				\
    }							\
} while(0)

#define PRINT_COLOR(color, r, R, d1, d2)			\
do {								\
  int comp0 = (32768 + ((ditherbit##d2 >> d->r##_randomizer) -	\
			(32768 >> d->r##_randomizer)));		\
  if (!l##color)						\
    {								\
      if (r > comp0)						\
	{							\
	  DO_PRINT_COLOR(r);					\
	  r -= 65535;						\
	}							\
    }								\
  else								\
    {								\
      int compare = (comp0 * d->l##r##_level) >> 16;		\
      if (r <= (d->l##r##_level))				\
	{							\
	  if (r > compare)					\
	    {							\
	      DO_PRINT_COLOR(l##r);				\
	      r -= d->l##r##_level;				\
	    }							\
	}							\
      else if (r > compare)					\
	{							\
	  int cutoff = ((density - d->l##r##_level) * 65536 /	\
			(65536 - d->l##r##_level));		\
	  long long sub;					\
	  if (cutoff >= 0)					\
	    sub = d->l##r##_level +				\
	      (((65535ll - d->l##r##_level) * cutoff) >> 16);	\
	  else							\
	    sub = d->l##r##_level +				\
	      ((65535ll - d->l##r##_level) * cutoff / 65536);	\
	  if (ditherbit##d1 > cutoff)				\
	    {							\
	      DO_PRINT_COLOR(l##r);				\
	    }							\
	  else							\
	    {							\
	      DO_PRINT_COLOR(r);				\
	    }							\
	  if (sub < d->l##r##_level)				\
	    r -= d->l##r##_level;				\
	  else if (sub > 65535)					\
	    r -= 65535;						\
	  else							\
	    r -= sub;						\
	}							\
    }								\
} while (0)

#define UPDATE_DITHER(r, d2, x, width)					 \
do {									 \
  int offset = (15 - (((o##r & 0xf000) >> 12)) * horizontal_overdensity) \
				       >> 1;				 \
  if (x < offset)							 \
    offset = x;								 \
  else if (x > dst_width - offset - 1)					 \
    offset = dst_width - x - 1;						 \
  if (ditherbit##d2 & bit)						 \
    {									 \
      r##error1[-offset] += r;						 \
      r##error1[0] += 3 * r;						 \
      r##error1[offset] += r;						 \
      dither##r    = r##error0[direction] + 3 * r;			 \
    }									 \
  else									 \
    {									 \
      r##error1[-offset] += r;						 \
      r##error1[0] +=  r;						 \
      r##error1[offset] += r;						 \
      dither##r    = r##error0[direction] + 5 * r;			 \
    }									 \
} while (0)

void
dither_cmyk(unsigned short  *rgb,	/* I - RGB pixels */
	    int           row,		/* I - Current Y coordinate */
	    int           src_width,	/* I - Width of input row */
	    int           dst_width,	/* I - Width of output rows */
	    unsigned char *cyan,	/* O - Cyan bitmap pixels */
	    unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
	    unsigned char *magenta,	/* O - Magenta bitmap pixels */
	    unsigned char *lmagenta,	/* O - Light magenta bitmap pixels */
	    unsigned char *yellow,	/* O - Yellow bitmap pixels */
	    unsigned char *lyellow,	/* O - Light yellow bitmap pixels */
	    unsigned char *black,	/* O - Black bitmap pixels */
	    int horizontal_overdensity)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  long long	c, m, y, k,	/* CMYK values */
		oc, om, ok, oy,
		divk;		/* Inverse of K */
  long long     diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*lmptr,		/* Current light magenta pixel */
		*lcptr,		/* Current light cyan pixel */
		*lyptr,		/* Current light yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */
  int nk;
  int ck;
  int bk;
  int ub, lb;
  int ditherbit0, ditherbit1, ditherbit2, ditherbit3;
  long long	density;
  dither_t      *d = &dither_info;

  /*
   * If horizontal_overdensity is > 1, we want to output a bit only so many
   * times that a bit would be generated.  These serve as counters for making
   * that decision.  We make these variable static rather than reinitializing
   * at zero each line to avoid having a line of bits near the edge of the
   * image.
   */
  int overdensity_bits = 0;

#ifdef PRINT_DEBUG
  long long odk, odc, odm, ody, dk, dc, dm, dy, xk, xc, xm, xy, yc, ym, yy;
  FILE *dbg;
#endif

  int terminate;
  int direction = row & 1 ? 1 : -1;

  switch (horizontal_overdensity)
    {
    case 0:
    case 1:
      overdensity_bits = 0;
      break;
    case 2:
      overdensity_bits = 1;
      break;
    case 4:
      overdensity_bits = 2;
      break;
    case 8:
      overdensity_bits = 3;
      break;
    }

  bit = (direction == 1) ? 128 : 1 << (7 - ((dst_width - 1) & 7));
  x = (direction == 1) ? 0 : dst_width - 1;
  terminate = (direction == 1) ? dst_width : -1;

  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][2];
  yerror1 = error[1 - (row & 1)][2];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];
  memset(kerror1, 0, dst_width * sizeof(int));
  memset(cerror1, 0, dst_width * sizeof(int));
  memset(merror1, 0, dst_width * sizeof(int));
  memset(yerror1, 0, dst_width * sizeof(int));
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      cerror0 += dst_width - 1;
      cerror1 += dst_width - 1;
      merror0 += dst_width - 1;
      merror1 += dst_width - 1;
      yerror0 += dst_width - 1;
      yerror1 += dst_width - 1;
      kerror0 += dst_width - 1;
      kerror1 += dst_width - 1;
      cptr = cyan + length - 1;
      if (lcptr)
	lcptr = lcyan + length - 1;
      mptr = magenta + length - 1;
      if (lmptr)
	lmptr = lmagenta + length - 1;
      yptr = yellow + length - 1;
      if (lyptr)
	lyptr = lyellow + length - 1;
      if (kptr)
	kptr = black + length - 1;
      xstep = -xstep;
      rgb += 3 * (src_width - 1);
      xerror = ((dst_width - 1) * xmod) % dst_width;
      xmod = -xmod;
    }

  memset(cyan, 0, length);
  if (lcyan)
    memset(lcyan, 0, length);
  memset(magenta, 0, length);
  if (lmagenta)
    memset(lmagenta, 0, length);
  memset(yellow, 0, length);
  if (lyellow)
    memset(lyellow, 0, length);
  if (black)
    memset(black, 0, length);

#ifdef PRINT_DEBUG
  dbg = fopen("/mnt1/dbg", "a");
#endif

  /*
   * Main loop starts here!
   */
  for (ditherbit = rand(),
	 ditherc = cerror0[0], ditherm = merror0[0], dithery = yerror0[0],
	 ditherk = kerror0[0],
	 ditherbit0 = ditherbit & 0xffff,
	 ditherbit1 = ((ditherbit >> 8) & 0xffff),
	 ditherbit2 = (((ditherbit >> 16) & 0x7fff) +
		       ((ditherbit & 0x100) << 7)),
	 ditherbit3 = (((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
		       ((ditherbit >> 8) & 0xff00));
       x != terminate;
       x += direction,
	 cerror0 += direction,
	 cerror1 += direction,
	 merror0 += direction,
	 merror1 += direction,
	 yerror0 += direction,
	 yerror1 += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {

   /*
    * First compute the standard CMYK separation color values...
    */
		   
    int maxlevel;
    int ak;
    int kdarkness;

    c = 65535 - (unsigned) rgb[0];
    m = 65535 - (unsigned) rgb[1];
    y = 65535 - (unsigned) rgb[2];
    oc = c;
    om = m;
    oy = y;
    k = MIN(c, MIN(m, y));
#ifdef PRINT_DEBUG
    xc = c;
    xm = m;
    xy = y;
    xk = k;
    yc = c;
    ym = m;
    yy = y;
#endif
    maxlevel = MAX(c, MAX(m, y));

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */
      int xdiff = (abs(c - m) + abs(c - y) + abs(m - y)) / 3;

      diff = 65536 - xdiff;
      diff = (diff * diff * diff) >> 32; /* diff = diff^3 */
      diff--;
      if (diff < 0)
	diff = 0;
      k    = (diff * k) >> 16;
      ak = k;
      divk = 65535 - k;
      if (divk == 0)
        c = m = y = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */

        c  = (65535 - ((rgb[2] + rgb[1]) >> 3)) * (c - k) / divk;
        m  = (65535 - ((rgb[1] + rgb[0]) >> 3)) * (m - k) / divk;
        y  = (65535 - ((rgb[0] + rgb[2]) >> 3)) * (y - k) / divk;
      }
#ifdef PRINT_DEBUG
      yc = c;
      ym = m;
      yy = y;
#endif

      /*
       * kdarkness is an artificially computed darkness value for deciding
       * how much black vs. CMY to use for the k component.  This is
       * empirically determined.
       */
      ok = k;
      nk = k + (ditherk) / 8;
      kdarkness = MAX((((c * d->c_darkness) +
			(m * d->m_darkness) +
			(y * d->y_darkness)) >> 6), ak);
      if (kdarkness < d->k_upper)
	{
	  int rb;
	  ub = d->k_upper;
	  lb = d->k_lower;
	  rb = ub - lb;
#ifdef PRINT_DEBUG
	  fprintf(dbg, "Black: kd %d ub %d lb %d rb %d test %d range %d\n",
		  kdarkness, ub, lb, rb, ditherbit % rb, kdarkness - lb);
#endif
	  if (kdarkness <= lb)
	    {
	      bk = 0;
	      ub = 0;
	      lb = 1;
	    }
	  else if (kdarkness < ub)
	    {
	      if (rb == 0 || (ditherbit % rb) < (kdarkness - lb))
		bk = nk;
	      else
		bk = 0;
	    }
	  else
	    {
	      ub = 1;
	      lb = 1;
	      bk = nk;
	    }
	}
      else
	{
#ifdef PRINT_DEBUG
	  fprintf(dbg, "Black real\n");
#endif
	  bk = nk;
	}
      ck = nk - bk;
    
      c += (d->k_clevel * ck) >> 4;
      m += (d->k_mlevel * ck) >> 4;
      y += (d->k_ylevel * ck) >> 4;

      /*
       * Don't allow cmy to grow without bound.
       */
      if (c > 65535)
	c = 65535;
      if (m > 65535)
	m = 65535;
      if (y > 65535)
	y = 65535;
      k = bk;
#ifdef PRINT_DEBUG
      odk = ditherk;
      dk = k;
#endif
      if (k > (32767 + ((ditherbit0 >> d->k_randomizer) -
			(32768 >> d->k_randomizer))))
	{
	  DO_PRINT_COLOR(k);
	  k -= 65535;
	}

      UPDATE_DITHER(k, 1, x, src_width);
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      ok = 0;
      c  = (65535 - rgb[1] / 4) * (c - k) / 65535 + k;
      m  = (65535 - rgb[2] / 4) * (m - k) / 65535 + k;
      y  = (65535 - rgb[0] / 4) * (y - k) / 65535 + k;
    }

    density = (c + m + y) >> overdensity_bits;
    UPDATE_COLOR(c);
    UPDATE_COLOR(m);
    UPDATE_COLOR(y);
    density += (c + m + y) >> overdensity_bits;
/*     density >>= 1; */

    if (!kptr || !(*kptr & bit))
      {
	PRINT_COLOR(cyan, c, C, 1, 2);
	PRINT_COLOR(magenta, m, M, 2, 3);
	PRINT_COLOR(yellow, y, Y, 3, 0);
      }

    UPDATE_DITHER(c, 2, x, dst_width);
    UPDATE_DITHER(m, 3, x, dst_width);
    UPDATE_DITHER(y, 0, x, dst_width);

    /*****************************************************************
     * Advance the loop
     *****************************************************************/
#ifdef PRINT_DEBUG
    fprintf(dbg, "   x %d y %d  r %d g %d b %d  xc %lld xm %lld xy %lld yc "
	    "%lld ym %lld yy %lld xk %lld  diff %lld divk %lld  oc %lld om "
	    "%lld oy %lld ok %lld  c %lld m %lld y %lld k %lld  %c%c%c%c%c%c%c"
	    "  dk %lld dc %lld dm %lld dy %lld  kd %d ck %d bk %d nk %d ub %d "
	    "lb %d\n",
	    x, row,
	    rgb[0], rgb[1], rgb[2],
	    xc, xm, xy, yc, ym, yy, xk, diff, divk,
	    oc, om, oy, ok,
	    dc, dm, dy, dk,
	    (*cptr & bit) ? 'c' : ' ',
	    (lcyan && (*lcptr & bit)) ? 'C' : ' ',
	    (*mptr & bit) ? 'm' : ' ',
	    (lmagenta && (*lmptr & bit)) ? 'M' : ' ',
	    (*yptr & bit) ? 'y' : ' ',
	    (lyellow && (*lyptr & bit)) ? 'Y' : ' ',
	    (black && (*kptr & bit)) ? 'k' : ' ',
	    odk, odc, odm, ody,
	    kdarkness, ck, bk, nk, ub, lb);
    fprintf(dbg, "x %d dir %d c %x %x m %x %x y %x %x k %x %x rgb %x bit %x\n",
	    x, direction, cptr, cyan, mptr, magenta, yptr, yellow, kptr, black,
	    rgb, bit);
#endif

    ditherbit = rand();
    ditherbit0 = ditherbit & 0xffff;
    ditherbit1 = ((ditherbit >> 8) & 0xffff);
    ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
    ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
      ((ditherbit >> 8) & 0xff00);
    if (direction == 1)
      {
	if (bit == 1)
	  {
	    cptr ++;
	    if (lcptr)
	      lcptr ++;
	    mptr ++;
	    if (lmptr)
	      lmptr ++;
	    yptr ++;
	    if (lyptr)
	      lyptr ++;
	    if (kptr)
	      kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    cptr --;
	    if (lcptr)
	      lcptr --;
	    mptr --;
	    if (lmptr)
	      lmptr --;
	    yptr --;
	    if (lyptr)
	      lyptr --;
	    if (kptr)
	      kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
      {
	xerror -= dst_width;
	rgb    += 3 * direction;
      }
    else if (xerror < 0)
      {
	xerror += dst_width;
	rgb    += 3 * direction;
      }      
  }
  /*
   * Main loop ends here!
   */
#ifdef PRINT_DEBUG
  fprintf(dbg, "\n");
  fclose(dbg);
#endif
}


/*
 * Constants for 4-level dithering functions...
 * NOTE that these constants are HP-specific!
 */

/*
 * 'dither_black4()' - Dither grayscale pixels to 4 levels of black.
 */

void
dither_black4(unsigned short    *gray,		/* I - Grayscale pixels */
	      int           	row,		/* I - Current Y coordinate */
	      int           	src_width,	/* I - Width of input row */
	      int           	dst_width,	/* I - Width of output row */
	      unsigned char 	*black,		/* O - Black bitmap pixels */
	      int horizontal_overdensity)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dithering bitmask */
  dither_t      *d = &dither_info;
  int overdensity_bits = 0;
  int terminate;
  int direction = row & 1 ? 1 : -1;

  switch (horizontal_overdensity)
    {
    case 0:
    case 1:
      overdensity_bits = 0;
      break;
    case 2:
      overdensity_bits = 1;
      break;
    case 4:
      overdensity_bits = 2;
      break;
    case 8:
      overdensity_bits = 3;
      break;
    }

  bit = (direction == 1) ? 128 : 1 << (7 - ((dst_width - 1) & 7));
  x = (direction == 1) ? 0 : dst_width - 1;
  terminate = (direction == 1) ? dst_width : -1;

  xstep  = src_width / dst_width;
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];
  memset(kerror1, 0, dst_width * sizeof(int));

  memset(black, 0, length);
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      kerror0 += dst_width - 1;
      kerror1 += dst_width - 1;
      kptr = black + length - 1;
      xstep = -xstep;
      gray += src_width - 1;
      xerror = ((dst_width - 1) * xmod) % dst_width;
      xmod = -xmod;
    }

  for (ditherbit = rand() & 0xffff, ditherk = kerror0[0];
       x != terminate;
       ditherbit = rand() & 0xffff,
	 x += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {
    int i;
    int offset;
    k = 65535 - *gray + ditherk / 8;
    offset = (15 - (((k & 0xf000) >> 12)) * horizontal_overdensity) >> 1;
    if (x < offset)
      offset = x;
    else if (x > dst_width - offset - 1)
      offset = dst_width - x - 1;

    for (i = d->nk_l; i > 0; i--)
      {
	if (k > d->k_transitions[i])
	  {
	    if (d->kbits++ == horizontal_overdensity)
	      {
		int j;
		for (j = 0; j < d->nm_log; j++)
		  {
		    if (j & i)
		      kptr[j * length] |= bit;
		  }
		d->kbits = 1;
	      }
	    k -= d->k_levels[i];
	    break;
	  }
      }

    if (ditherbit & bit)
    {
      kerror1[-offset] += k;
      kerror1[0] += 3 * k;
      kerror1[0] += k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[-offset] += k;
      kerror1[0] += k;
      kerror1[0] += k;
      ditherk    = kerror0[1] + 5 * k;
    }

    if (direction == 1)
      {
	if (bit == 1)
	  {
	    kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    gray   += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
      {
	xerror -= dst_width;
	gray   += direction;
      }
    else if (xerror < 0)
      {
	xerror += dst_width;
	gray   += direction;
      }      
  }
}

/*
 * 'dither_cmyk4()' - Dither RGB pixels to 4 levels of cyan, magenta, yellow,
 *                    and black.
 */

#define DO_PRINT_COLOR_4(base, r, ratio)		\
do {							\
  int i;						\
  for (i = d->n##r##_l; i > 0; i--)			\
    {							\
      if (base > d->r##_transitions[i])			\
	{						\
	  if (d->r##bits++ == horizontal_overdensity)	\
	    {						\
	      int j;					\
	      for (j = 0; j < d->n##r##_log; j++)	\
		{					\
		  if (j & i)				\
		    r##ptr[j * length] |= bit;		\
		}					\
	      d->r##bits = 1;				\
	    }						\
	  base -= d->r##_levels[i];			\
	  break;					\
	}						\
    }							\
} while (0)

#define PRINT_COLOR_4(color, r, R, d1, d2)				\
do {									\
  int comp0 = (32768 + ((ditherbit##d2 >> d->r##_randomizer) -		\
			(32768 >> d->r##_randomizer)));			\
  if (!l##color)							\
    {									\
      DO_PRINT_COLOR_4(r, r, 1);					\
    }									\
  else									\
    {									\
      int compare = comp0 * d->l##r##_level >> 16;			\
      if (r <= (d->l##r##_level))					\
	{								\
	  if (r > compare)						\
	    {								\
	      DO_PRINT_COLOR_4(r, l##r, d->l##r##_level / 65536);	\
	    }								\
	}								\
      else if (r > compare)						\
	{								\
	  int cutoff = ((density - d->l##r##_level) * 65536 /		\
			(65536 - d->l##r##_level));			\
	  long long sub;						\
	  if (cutoff >= 0)						\
	    sub = d->l##r##_level +					\
	      (((65535ll - d->l##r##_level) * cutoff) >> 16);		\
	  else								\
	    sub = d->l##r##_level +					\
	      ((65535ll - d->l##r##_level) * cutoff / 65536);		\
	  if (ditherbit##d1 > cutoff)					\
	    {								\
	      DO_PRINT_COLOR_4(r, l##r, d->l##r##_level / 65536);	\
	    }								\
	  else								\
	    {								\
	      DO_PRINT_COLOR_4(r, r, 1);				\
	    }								\
	}								\
    }									\
} while (0)

void
dither_cmyk4(unsigned short  *rgb,	/* I - RGB pixels */
	     int           row,		/* I - Current Y coordinate */
	     int           src_width,	/* I - Width of input row */
	     int           dst_width,	/* I - Width of output rows */
	     unsigned char *cyan,	/* O - Cyan bitmap pixels */
	     unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
	     unsigned char *magenta,	/* O - Magenta bitmap pixels */
	     unsigned char *lmagenta,	/* O - Light magenta bitmap pixels */
	     unsigned char *yellow,	/* O - Yellow bitmap pixels */
	     unsigned char *lyellow,	/* O - Light yellow bitmap pixels */
	     unsigned char *black,	/* O - Black bitmap pixels */
	     int horizontal_overdensity)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  long long	c, m, y, k,	/* CMYK values */
		oc, om, ok, oy,
		divk;		/* Inverse of K */
  long long     diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*lmptr,		/* Current light magenta pixel */
		*lcptr,		/* Current light cyan pixel */
		*lyptr,		/* Current light yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */
  int nk;
  int ck;
  int bk;
  int ub, lb;
  int ditherbit0, ditherbit1, ditherbit2, ditherbit3;
  long long	density;
  dither_t      *d = &dither_info;

  /*
   * If horizontal_overdensity is > 1, we want to output a bit only so many
   * times that a bit would be generated.  These serve as counters for making
   * that decision.  We make these variable static rather than reinitializing
   * at zero each line to avoid having a line of bits near the edge of the
   * image.
   */
  int overdensity_bits = 0;

  int terminate;
  int direction = row & 1 ? 1 : -1;

  switch (horizontal_overdensity)
    {
    case 0:
    case 1:
      overdensity_bits = 0;
      break;
    case 2:
      overdensity_bits = 1;
      break;
    case 4:
      overdensity_bits = 2;
      break;
    case 8:
      overdensity_bits = 3;
      break;
    }

  bit = (direction == 1) ? 128 : 1 << (7 - ((dst_width - 1) & 7));
  x = (direction == 1) ? 0 : dst_width - 1;
  terminate = (direction == 1) ? dst_width : -1;

  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][2];
  yerror1 = error[1 - (row & 1)][2];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];
  memset(kerror1, 0, dst_width * sizeof(int));
  memset(cerror1, 0, dst_width * sizeof(int));
  memset(merror1, 0, dst_width * sizeof(int));
  memset(yerror1, 0, dst_width * sizeof(int));
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      cerror0 += dst_width - 1;
      cerror1 += dst_width - 1;
      merror0 += dst_width - 1;
      merror1 += dst_width - 1;
      yerror0 += dst_width - 1;
      yerror1 += dst_width - 1;
      kerror0 += dst_width - 1;
      kerror1 += dst_width - 1;
      cptr = cyan + length - 1;
      if (lcptr)
	lcptr = lcyan + length - 1;
      mptr = magenta + length - 1;
      if (lmptr)
	lmptr = lmagenta + length - 1;
      yptr = yellow + length - 1;
      if (lyptr)
	lyptr = lyellow + length - 1;
      if (kptr)
	kptr = black + length - 1;
      xstep = -xstep;
      rgb += 3 * (src_width - 1);
      xerror = ((dst_width - 1) * xmod) % dst_width;
      xmod = -xmod;
    }

  memset(cyan, 0, length);
  if (lcyan)
    memset(lcyan, 0, length);
  memset(magenta, 0, length);
  if (lmagenta)
    memset(lmagenta, 0, length);
  memset(yellow, 0, length);
  if (lyellow)
    memset(lyellow, 0, length);
  if (black)
    memset(black, 0, length);

  /*
   * Main loop starts here!
   */
  for (ditherbit = rand(),
	 ditherc = cerror0[0], ditherm = merror0[0], dithery = yerror0[0],
	 ditherk = kerror0[0],
	 ditherbit0 = ditherbit & 0xffff,
	 ditherbit1 = ((ditherbit >> 8) & 0xffff),
	 ditherbit2 = (((ditherbit >> 16) & 0x7fff) +
		       ((ditherbit & 0x100) << 7)),
	 ditherbit3 = (((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
		       ((ditherbit >> 8) & 0xff00));
       x != terminate;
       x += direction,
	 cerror0 += direction,
	 cerror1 += direction,
	 merror0 += direction,
	 merror1 += direction,
	 yerror0 += direction,
	 yerror1 += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {

   /*
    * First compute the standard CMYK separation color values...
    */
		   
    int maxlevel;
    int ak;
    int kdarkness;

    c = 65535 - (unsigned) rgb[0];
    m = 65535 - (unsigned) rgb[1];
    y = 65535 - (unsigned) rgb[2];
    oc = c;
    om = m;
    oy = y;
    k = MIN(c, MIN(m, y));
    maxlevel = MAX(c, MAX(m, y));

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */
      int xdiff = (abs(c - m) + abs(c - y) + abs(m - y)) / 3;

      diff = 65536 - xdiff;
      diff = (diff * diff * diff) >> 32; /* diff = diff^3 */
      diff--;
      if (diff < 0)
	diff = 0;
      k    = (diff * k) >> 16;
      ak = k;
      divk = 65535 - k;
      if (divk == 0)
        c = m = y = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */

        c  = (65535 - ((rgb[2] + rgb[1]) >> 3)) * (c - k) / divk;
        m  = (65535 - ((rgb[1] + rgb[0]) >> 3)) * (m - k) / divk;
        y  = (65535 - ((rgb[0] + rgb[2]) >> 3)) * (y - k) / divk;
      }

      /*
       * kdarkness is an artificially computed darkness value for deciding
       * how much black vs. CMY to use for the k component.  This is
       * empirically determined.
       */
      ok = k;
      nk = k + (ditherk) / 8;
      kdarkness = MAX((((c * d->c_darkness) +
			(m * d->m_darkness) +
			(y * d->y_darkness)) >> 6), ak);
      if (kdarkness < d->k_upper)
	{
	  int rb;
	  ub = d->k_upper;
	  lb = d->k_lower;
	  rb = ub - lb;
	  if (kdarkness <= lb)
	    {
	      bk = 0;
	      ub = 0;
	      lb = 1;
	    }
	  else if (kdarkness < ub)
	    {
	      if (rb == 0 || (ditherbit % rb) < (kdarkness - lb))
		bk = nk;
	      else
		bk = 0;
	    }
	  else
	    {
	      ub = 1;
	      lb = 1;
	      bk = nk;
	    }
	}
      else
	{
	  bk = nk;
	}
      ck = nk - bk;
    
      /*
       * These constants are empirically determined to produce a CMY value
       * that looks reasonably gray and is reasonably well balanced tonally
       * with black.  As usual, this is very ad hoc and needs to be
       * generalized.
       */
      c += (d->k_clevel * ck) >> 4;
      m += (d->k_mlevel * ck) >> 4;
      y += (d->k_ylevel * ck) >> 4;

      /*
       * Don't allow cmy to grow without bound.
       */
      if (c > 65535)
	c = 65535;
      if (m > 65535)
	m = 65535;
      if (y > 65535)
	y = 65535;
      k = bk;
      if (k > (32767 + ((ditherbit0 >> d->k_randomizer) -
			(32768 >> d->k_randomizer))))
	DO_PRINT_COLOR_4(k, k, 1);
      UPDATE_DITHER(k, 1, x, src_width);
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      ok = 0;
      c  = (65535 - rgb[1] / 4) * (c - k) / 65535 + k;
      m  = (65535 - rgb[2] / 4) * (m - k) / 65535 + k;
      y  = (65535 - rgb[0] / 4) * (y - k) / 65535 + k;
    }

    density = (c + m + y) >> overdensity_bits;
    UPDATE_COLOR(c);
    UPDATE_COLOR(m);
    UPDATE_COLOR(y);
    density += (c + m + y) >> overdensity_bits;
/*     density >>= 1; */

    if (!kptr || !(*kptr & bit))
      {
	PRINT_COLOR_4(cyan, c, C, 1, 2);
	PRINT_COLOR_4(magenta, m, M, 2, 3);
	PRINT_COLOR_4(yellow, y, Y, 3, 0);
      }

    UPDATE_DITHER(c, 2, x, dst_width);
    UPDATE_DITHER(m, 3, x, dst_width);
    UPDATE_DITHER(y, 0, x, dst_width);

    /*****************************************************************
     * Advance the loop
     *****************************************************************/

    ditherbit = rand();
    ditherbit0 = ditherbit & 0xffff;
    ditherbit1 = ((ditherbit >> 8) & 0xffff);
    ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
    ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
      ((ditherbit >> 8) & 0xff00);
    if (direction == 1)
      {
	if (bit == 1)
	  {
	    cptr ++;
	    if (lcptr)
	      lcptr ++;
	    mptr ++;
	    if (lmptr)
	      lmptr ++;
	    yptr ++;
	    if (lyptr)
	      lyptr ++;
	    if (kptr)
	      kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    cptr --;
	    if (lcptr)
	      lcptr --;
	    mptr --;
	    if (lmptr)
	      lmptr --;
	    yptr --;
	    if (lyptr)
	      lyptr --;
	    if (kptr)
	      kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
      {
	xerror -= dst_width;
	rgb    += 3 * direction;
      }
    else if (xerror < 0)
      {
	xerror += dst_width;
	rgb    += 3 * direction;
      }      
  }
  /*
   * Main loop ends here!
   */
}


/* rgb/hsv conversions taken from Gimp common/autostretch_hsv.c */


static void
calc_rgb_to_hsv(unsigned short *rgb, double *hue, double *sat, double *val)
{
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = rgb[0] / 65535.0;
  green = rgb[1] / 65535.0;
  blue  = rgb[2] / 65535.0;

  h = 0.0; /* Shut up -Wall */

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    h = 0.0;
  else
    {
      delta = max - min;

      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *hue = h;
  *sat = s;
  *val = v;
}

static void
calc_hsv_to_rgb(unsigned short *rgb, double h, double s, double v)
{
  double hue, saturation, value;
  double f, p, q, t;

  if (s == 0.0)
    {
      h = v;
      s = v;
      v = v; /* heh */
    }
  else
    {
      hue        = h * 6.0;
      saturation = s;
      value      = v;

      if (hue == 6.0)
	hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
	{
	case 0:
	  h = value;
	  s = t;
	  v = p;
	  break;

	case 1:
	  h = q;
	  s = value;
	  v = p;
	  break;

	case 2:
	  h = p;
	  s = value;
	  v = t;
	  break;

	case 3:
	  h = p;
	  s = q;
	  v = value;
	  break;

	case 4:
	  h = t;
	  s = p;
	  v = value;
	  break;

	case 5:
	  h = value;
	  s = p;
	  v = q;
	  break;
	}
    }

  rgb[0] = h*65535;
  rgb[1] = s*65535;
  rgb[2] = v*65535;
  
}


/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */

void
gray_to_gray(unsigned char *grayin,	/* I - RGB pixels */
	     unsigned short *grayout,	/* O - RGB pixels */
	     int    	width,		/* I - Width of row */
	     int    	bpp,		/* I - Bytes-per-pixel in grayin */
	     lut_t  	*lut,		/* I - Brightness lookup table */
	     unsigned char *cmap,	/* I - Colormap (unused) */
	     vars_t	*vars
	     )
{
  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[*grayin];

      grayin ++;
      grayout ++;
      width --;
    }
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[grayin[0] * grayin[1] / 255] + 255 - grayin[1];

      grayin += bpp;
      grayout ++;
      width --;
    }
  }
}


/*
 * 'indexed_to_gray()' - Convert indexed image data to grayscale.
 */

void
indexed_to_gray(unsigned char *indexed,		/* I - Indexed pixels */
		  unsigned short *gray,		/* O - Grayscale pixels */
		  int    width,			/* I - Width of row */
		  int    bpp,			/* I - bpp in indexed */
		  lut_t  *lut,			/* I - Brightness LUT */
		  unsigned char *cmap,		/* I - Colormap */
		  vars_t   *vars		/* I - Saturation */
		  )
{
  int		i;			/* Looping var */
  unsigned char	gray_cmap[256];		/* Grayscale colormap */


  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[*indexed]];
      indexed ++;
      gray ++;
      width --;
    }
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[indexed[0] * indexed[1] / 255] +
			    255 - indexed[1]];
      indexed += bpp;
      gray ++;
      width --;
    }
  }
}


void
indexed_to_rgb(unsigned char *indexed,	/* I - Indexed pixels */
		 unsigned short *rgb,	/* O - RGB pixels */
		 int    width,		/* I - Width of row */
		 int    bpp,		/* I - Bytes-per-pixel in indexed */
		 lut_t  *lut,		/* I - Brightness lookup table */
		 unsigned char *cmap,	/* I - Colormap */
		 vars_t   *vars	/* I - Saturation */
		 )
{
  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      double h, s, v;
      rgb[0] = lut->red[cmap[*indexed * 3 + 0]];
      rgb[1] = lut->green[cmap[*indexed * 3 + 1]];
      rgb[2] = lut->blue[cmap[*indexed * 3 + 2]];
      if (vars->saturation != 1.0)
	{
	  calc_rgb_to_hsv(rgb, &h, &s, &v);
	  s = pow(s, 1.0 / vars->saturation);
	  calc_hsv_to_rgb(rgb, h, s, v);
	}
      rgb += 3;
      indexed ++;
      width --;
    }
  }
  else
  {
   /*
    * RGBA image...
    */

    while (width > 0)
    {
      double h, s, v;
      rgb[0] = lut->red[cmap[indexed[0] * 3 + 0] * indexed[1] / 255 +
		       255 - indexed[1]];
      rgb[1] = lut->green[cmap[indexed[0] * 3 + 1] * indexed[1] / 255 +
			 255 - indexed[1]];
      rgb[2] = lut->blue[cmap[indexed[0] * 3 + 2] * indexed[1] / 255 +
			255 - indexed[1]];
      if (vars->saturation != 1.0)
	{
	  calc_rgb_to_hsv(rgb, &h, &s, &v);
	  s = pow(s, 1.0 / vars->saturation);
	  calc_hsv_to_rgb(rgb, h, s, v);
	}
      rgb += 3;
      indexed += bpp;
      width --;
    }
  }
}



/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

void
rgb_to_gray(unsigned char *rgb,		/* I - RGB pixels */
	      unsigned short *gray,	/* O - Grayscale pixels */
	      int    width,		/* I - Width of row */
	      int    bpp,		/* I - Bytes-per-pixel in RGB */
	      lut_t  *lut,		/* I - Brightness lookup table */
	      unsigned char *cmap,	/* I - Colormap (unused) */
	      vars_t   *vars		/* I - Saturation */
	      )
{
  if (bpp == 3)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[(rgb[0] * LUM_RED +
			      rgb[1] * LUM_GREEN +
			      rgb[2] * LUM_BLUE) / 100];
      gray ++;
      rgb += 3;
      width --;
    }
  }
  else
  {
   /*
    * Image has alpha channel...
    */

    while (width > 0)
    {
      *gray = lut->composite[((rgb[0] * LUM_RED +
			       rgb[1] * LUM_GREEN +
			       rgb[2] * LUM_BLUE) *
			      rgb[3] / 25500 + 255 - rgb[3])];
      gray ++;
      rgb += bpp;
      width --;
    }
  }
}

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

void
rgb_to_rgb(unsigned char	*rgbin,		/* I - RGB pixels */
	   unsigned short 	*rgbout,	/* O - RGB pixels */
	   int    		width,		/* I - Width of row */
	   int    		bpp,		/* I - Bytes/pix in indexed */
	   lut_t 		*lut,		/* I - Brightness LUT */
	   unsigned char 	*cmap,		/* I - Colormap */
	   vars_t  		*vars		/* I - Saturation */
	   )
{
  if (bpp == 3)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      double h, s, v;
      rgbout[0] = lut->red[rgbin[0]];
      rgbout[1] = lut->green[rgbin[1]];
      rgbout[2] = lut->blue[rgbin[2]];
      if (vars->saturation != 1.0 || vars->contrast != 100)
	{
	  calc_rgb_to_hsv(rgbout, &h, &s, &v);
	  if (vars->saturation != 1.0)
	    s = pow(s, 1.0 / vars->saturation);
#if 0
	  if (vars->contrast != 100)
	    {
	      double contrast = vars->contrast / 100.0;
	      double tv = fabs(v - .5) * 2.0;
	      tv = pow(tv, 1.0 / (contrast * contrast));
	      if (v < .5)
		tv = - tv;
	      v = (tv / 2.0) + .5;
	    }
#endif
	  calc_hsv_to_rgb(rgbout, h, s, v);
	}
      if (vars->density != 1.0)
	{
	  float t;
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      t = ((float) rgbout[i]) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      rgbout[i] = (unsigned short) (t * 65536.0);
	    }
	}
      rgbin += 3;
      rgbout += 3;
      width --;
    }
  }
  else
  {
   /*
    * RGBA image...
    */

    while (width > 0)
    {
      double h, s, v;
      rgbout[0] = lut->red[rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[1] = lut->green[rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[2] = lut->blue[rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3]];
      if (vars->saturation != 1.0 || vars->contrast != 100 ||
	  vars->density != 1.0)
	{
	  calc_rgb_to_hsv(rgbout, &h, &s, &v);
	  if (vars->saturation != 1.0)
	    s = pow(s, 1.0 / vars->saturation);
#if 0
	  if (vars->contrast != 100)
	    {
	      double contrast = vars->contrast / 100.0;
	      double tv = fabs(v - .5) * 2.0;
	      tv = pow(tv, 1.0 / (contrast * contrast));
	      if (v < .5)
		tv = - tv;
	      v = (tv / 2.0) + .5;
	    }
#endif
	  calc_hsv_to_rgb(rgbout, h, s, v);
	}
      if (vars->density != 1.0)
	{
	  float t;
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      t = ((float) rgbout[i]) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      rgbout[i] = (unsigned short) (t * 65536.0);
	    }
	}
      rgbin += bpp;
      rgbout += 3;
      width --;
    }
  }
}

/* #define PRINT_LUT */

void
compute_lut(lut_t *lut,
	    float print_gamma,
	    float app_gamma,
	    vars_t *v)
{
  float		brightness,	/* Computed brightness */
		screen_gamma,	/* Screen gamma correction */
		pixel,		/* Pixel value */
		red_pixel,	/* Pixel value */
		green_pixel,	/* Pixel value */
		blue_pixel;	/* Pixel value */
  int i;
#ifdef PRINT_LUT
  FILE *ltfile = fopen("/mnt1/lut", "w");
#endif
  /*
   * Got an output file/command, now compute a brightness lookup table...
   */

  float red = 100.0 / v->red ;
  float green = 100.0 / v->green;
  float blue = 100.0 / v->blue;
  float contrast;
  contrast = v->contrast / 100.0;
  if (red < 0.01)
    red = 0.01;
  if (green < 0.01)
    green = 0.01;
  if (blue < 0.01)
    blue = 0.01;

  if (v->linear)
    {
      screen_gamma = app_gamma / 1.7;
      brightness   = v->brightness / 100.0;
    }
  else
    {
      brightness   = 100.0 / v->brightness;
      screen_gamma = app_gamma * brightness / 1.7;
    }

  print_gamma = v->gamma / print_gamma;

  for (i = 0; i < 256; i ++)
    {
      if (v->linear)
	{
	  double adjusted_pixel;
	  pixel = adjusted_pixel = (float) i / 255.0;

	  if (brightness < 1.0)
	    adjusted_pixel = adjusted_pixel * brightness;
	  else if (brightness > 1.0)
	    adjusted_pixel = 1.0 - ((1.0 - adjusted_pixel) / brightness);

	  if (pixel < 0)
	    adjusted_pixel = 0;
	  else if (pixel > 1.0)
	    adjusted_pixel = 1.0;

	  adjusted_pixel = pow(adjusted_pixel,
			       print_gamma * screen_gamma * print_gamma);

	  adjusted_pixel *= 65535.0;

	  red_pixel = green_pixel = blue_pixel = adjusted_pixel;
	  lut->composite[i] = adjusted_pixel;
	  lut->red[i] = adjusted_pixel;
	  lut->green[i] = adjusted_pixel;
	  lut->blue[i] = adjusted_pixel;
	}
      else
	{
	  float temp_pixel;
	  pixel = (float) i / 255.0;

	  /*
	   * First, correct contrast
	   */
	  temp_pixel = fabs((pixel - .5) * 2.0);
	  temp_pixel = pow(temp_pixel, 1.0 / contrast);
	  if (pixel < .5)
	    temp_pixel = -temp_pixel;
	  pixel = (temp_pixel / 2.0) + .5;

	  /*
	   * Second, perform screen gamma correction
	   */
	  pixel = 1.0 - pow(pixel, screen_gamma);

	  /*
	   * Third, fix up red, green, blue values
	   *
	   * I don't know how to do this correctly.  I think that what I'll do
	   * is if the correction is less than 1 to multiply it by the
	   * correction; if it's greater than 1, hinge it around 64K.
	   * Doubtless we can do better.  Oh well.
	   */
	  if (pixel < 0.0)
	    pixel = 0.0;
	  else if (pixel > 1.0)
	    pixel = 1.0;

	  red_pixel = pow(pixel, 1.0 / (red * red));
	  green_pixel = pow(pixel, 1.0 / (green * green));
	  blue_pixel = pow(pixel, 1.0 / (blue * blue));

	  /*
	   * Finally, fix up print gamma and scale
	   */

	  pixel = 256.0 * (256.0 - 256.0 *
			   pow(pixel, print_gamma));
	  red_pixel = 256.0 * (256.0 - 256.0 *
			       pow(red_pixel, print_gamma));
	  green_pixel = 256.0 * (256.0 - 256.0 *
				 pow(green_pixel, print_gamma));
	  blue_pixel = 256.0 * (256.0 - 256.0 *
				pow(blue_pixel, print_gamma));

	  if (pixel <= 0.0)
	    lut->composite[i] = 0;
	  else if (pixel >= 65535.0)
	    lut->composite[i] = 65535;
	  else
	    lut->composite[i] = (unsigned)(pixel);

	  if (red_pixel <= 0.0)
	    lut->red[i] = 0;
	  else if (red_pixel >= 65535.0)
	    lut->red[i] = 65535;
	  else
	    lut->red[i] = (unsigned)(red_pixel);

	  if (green_pixel <= 0.0)
	    lut->green[i] = 0;
	  else if (green_pixel >= 65535.0)
	    lut->green[i] = 65535;
	  else
	    lut->green[i] = (unsigned)(green_pixel);

	  if (blue_pixel <= 0.0)
	    lut->blue[i] = 0;
	  else if (blue_pixel >= 65535.0)
	    lut->blue[i] = 65535;
	  else
	    lut->blue[i] = (unsigned)(blue_pixel);
	}
#ifdef PRINT_LUT
      fprintf(ltfile, "%3i  %5d  %5d  %5d  %5d  %f %f %f %f  %f %f %f  %f\n",
	      i, lut->composite[i], lut->red[i], lut->green[i],
	      lut->blue[i], pixel, red_pixel, green_pixel, blue_pixel,
	      print_gamma, screen_gamma, print_gamma, app_gamma);
#endif
    }

#ifdef PRINT_LUT
  fclose(ltfile);
#endif
}

/*
 * 'default_media_size()' - Return the size of a default page size.
 */

const static papersize_t paper_sizes[] =
{
  { "Postcard", 283,  416 },
  { "4x6",	288,  432 },
  { "A6", 	295,  417 },
  { "5x8", 	360,  576 },
  { "A5", 	424,  597 },
  { "B5", 	518,  727 },
  { "8x10", 	576,  720 },
  { "A4", 	595,  842 },
  { "Letter", 	612,  792 },
  { "Legal", 	612,  1008 },
  { "B4", 	727,  1029 },
  { "Tabloid",  792,  1214 },
  { "A3", 	842,  1191 },
  { "12x18", 	864,  1296 },
  { "13x19", 	936,  1368 },
  { "A2", 	1188, 1684 },
  { "", 	0,    0 }
};

int
known_papersizes(void)
{
  return sizeof(paper_sizes) / sizeof(papersize_t);
}

const papersize_t *
get_papersizes(void)
{
  return paper_sizes;
}

const papersize_t *
get_papersize_by_name(const char *name)
{
  const papersize_t *val = &(paper_sizes[0]);
  while (strlen(val->name) > 0)
    {
      if (!strcmp(val->name, name))
	return val;
      val++;
    }
  return NULL;
}

void
default_media_size(int  model,		/* I - Printer model */
        	   char *ppd_file,	/* I - PPD file (not used) */
        	   char *media_size,	/* I - Media size */
        	   int  *width,		/* O - Width in points */
        	   int  *length)	/* O - Length in points */
{
  const papersize_t *papersize = get_papersize_by_name(media_size);
  if (!papersize)
    {
      *width = 0;
      *length = 0;
    }
  else
    {
      *width = papersize->width;
      *length = papersize->length;
    }
}

const static printer_t	printers[] =	/* List of supported printer types */
{
  { "PostScript Level 1",	"ps",		1,	0,	1.000,	1.000,
    ps_parameters,	ps_media_size,	ps_imageable_area,	ps_print },
  { "PostScript Level 2",	"ps2",		1,	1,	1.000,	1.000,
    ps_parameters,	ps_media_size,	ps_imageable_area,	ps_print },
  { "HP DeskJet 500, 520",	"pcl-500",	0,	500,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 500C, 540C",	"pcl-501",	1,	501,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 550C, 560C",	"pcl-550",	1,	550,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 600 series",	"pcl-600",	1,	600,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 800 series",	"pcl-800",	1,	800,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 1100C, 1120C",	"pcl-1100",	1,	1100,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 1200C, 1600C",	"pcl-1200",	1,	1200,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet II series",	"pcl-2",	0,	2,	1.000,	0.596,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet III series",	"pcl-3",	0,	3,	1.000,	0.596,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 4 series",	"pcl-4",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 4V, 4Si",	"pcl-4v",	0,	5,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 5 series",	"pcl-5",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 5Si",		"pcl-5si",	0,	5,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 6 series",	"pcl-6",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "EPSON Stylus Color",	"escp2",	1,	0,	0.597,	0.568,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color Pro",	"escp2-pro",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color Pro XL","escp2-proxl",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 1500",	"escp2-1500",	1,	2,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 400",	"escp2-400",	1,	1,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 440",	"escp2-440",	1,	9,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 500",	"escp2-500",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 600",	"escp2-600",	1,	3,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 640",	"escp2-640",	1,	10,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 740",	"escp2-740",	1,	11,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 800",	"escp2-800",	1,	4,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 850",	"escp2-850",	1,	15,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 860",	"escp2-860",	1,	16,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 900",	"escp2-900",	1,	12,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 1520",	"escp2-1520",	1,	5,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 3000",	"escp2-3000",	1,	5,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 700",	"escp2-700",	1,	6,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo EX",	"escp2-ex",	1,	7,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 750",	"escp2-750",	1,	13,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 870",	"escp2-870",	1,	13,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 1200",	"escp2-1200",	1,	14,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 1270",	"escp2-1270",	1,	14,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo",	"escp2-photo",	1,	8,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },

  { "CANON BJC 1000",           "bjc-1000",     1,      1000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 2000",           "bjc-2000",     1,      2000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 3000",           "bjc-3000",     1,      3000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 6000",           "bjc-6000",     1,      6000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 6100",           "bjc-6100",     1,      6100,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 7000",           "bjc-7000",     1,      7000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 7100",           "bjc-7100",     1,      7100,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
};


int
known_printers(void)
{
  return sizeof(printers) / sizeof(printer_t);
}

const printer_t *
get_printers(void)
{
  return printers;
}

const printer_t *
get_printer_by_index(int idx)
{
  return &(printers[idx]);
}

const printer_t *
get_printer_by_long_name(const char *long_name)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->long_name, long_name))
	return val;
      val++;
    }
  return NULL;
}

const printer_t *
get_printer_by_driver(const char *driver)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->driver, driver))
	return val;
      val++;
    }
  return NULL;
}

int
get_printer_index_by_driver(const char *driver)
{
  int idx = 0;
  const printer_t *val = &(printers[0]);
  for (idx = 0; idx < known_printers(); idx++)
    {
      if (!strcmp(val->driver, driver))
	return idx;
      val++;
    }
  return -1;
}

/*
 * End of "$Id$".
 */
