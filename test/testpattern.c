#include <gimp-print.h>
#include <varargs.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static char *
c_strdup(const char *s)
{
  char *ret = malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

double c_level = 1.0;
double m_level = 1.0;
double y_level = 1.0;
int levels = 256;
double ink_limit = 65535.0;
int width, height, bandheight;
int printer_width, printer_height;

static const char *Image_get_appname(stp_image_t *image);
static void Image_progress_conclude(stp_image_t *image);
static void Image_note_progress(stp_image_t *image,
				double current, double total);
static void Image_progress_init(stp_image_t *image);
static stp_image_status_t Image_get_row(stp_image_t *image,
					unsigned char *data, int row);
static int Image_height(stp_image_t *image);
static int Image_width(stp_image_t *image);
static int Image_bpp(stp_image_t *image);
static void Image_rotate_180(stp_image_t *image);
static void Image_rotate_cw(stp_image_t *image);
static void Image_rotate_ccw(stp_image_t *image);
static void Image_init(stp_image_t *image);
extern const int n_fillfuncs;

static void
do_help(void)
{
  fprintf(stderr, "%s", "\
Usage: testpattern -p printer [-n ramp_levels] [-l ink_limit] [-i ink_type]\n\
                   [-r resolution] [-s media_source] [-t media_type]\n\
                   [-z media_size] [-d dither_algorithm] [-e density]\n\
                   [-C cyan_level] [-M magenta_level] [-Y yellow_level]\n\
       0.0 < ink_limit <= 1.0\n\
       1 < ramp_levels <= 4096\n\
       0.1 <= density <= 2.0\n\
       0.0 < cyan_level <= 10.0 same for magenta and yellow.\n");
  exit(1);
}

static void
writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

static stp_image_t theImage =
{
  Image_init,
  NULL,				/* reset */
  NULL,				/* transpose */
  NULL,				/* hflip */
  NULL,				/* vflip */
  NULL,				/* crop */
  Image_rotate_ccw,
  Image_rotate_cw,
  Image_rotate_180,
  Image_bpp,
  Image_width,
  Image_height,
  Image_get_row,
  Image_get_appname,
  Image_progress_init,
  Image_note_progress,
  Image_progress_conclude,
  NULL
};

typedef const char *(*defparm_t)(const stp_printer_t printer,
				 const char *ppd_file,
				 const char *name);

int
main(int argc, char **argv)
{
  char *printer = 0;
  char *ink_type = 0;
  char *resolution = 0;
  char *media_source = 0;
  char *media_type = 0;
  char *media_size = 0;
  char *dither_algorithm = 0;
  int levels = 256;
  double density = 1.0;
  int c;
  stp_vars_t v;
  stp_printer_t the_printer;
  int left, right, top, bottom;
  const stp_printfuncs_t *printfuncs;
  defparm_t defparms;
  int x, y, oheight, owidth;

  while (1)
    {
      c = getopt(argc, argv, "p:n:l:i:r:s:t:z:d:hC:M:Y:e:");
      if (c == -1)
	break;
      switch (c)
	{
	case 'p':
	  printer = c_strdup(optarg);
	  break;
	case 'n':
	  levels = atoi(optarg);
	  break;
	case 'l':
	  ink_limit = 65535.0 * strtod(optarg, 0);
	  break;
	case 'i':
	  ink_type = c_strdup(optarg);
	  break;
	case 'r':
	  resolution = c_strdup(optarg);
	  break;
	case 's':
	  media_source = c_strdup(optarg);
	  break;
	case 't':
	  media_type = c_strdup(optarg);
	  break;
	case 'z':
	  media_size = c_strdup(optarg);
	  break;
	case 'd':
	  dither_algorithm = c_strdup(optarg);
	  break;
	case 'C':
	  c_level = strtod(optarg, 0);
	  break;
	case 'M':
	  m_level = strtod(optarg, 0);
	  break;
	case 'Y':
	  y_level = strtod(optarg, 0);
	  break;
	case 'e':
	  density = strtod(optarg, 0);
	  break;
	case 'h':
	  do_help();
	  break;
	default:
	  fprintf(stderr, "Unknown option '-%c'\n", c);
	  do_help();
	  break;
	}
    }
  if (!printer ||
      ink_limit <= 0 || ink_limit > 65535.0 ||
      levels < 1 || levels > 4096 ||
      c_level <= 0 || c_level > 10 ||
      m_level <= 0 || m_level > 10 ||
      y_level <= 0 || y_level > 10 ||
      density < .1 || density > 2.0)
    do_help();
  stp_init();
  v = stp_allocate_vars();
  the_printer = stp_get_printer_by_driver(printer);
  if (!the_printer)
    {
      the_printer = stp_get_printer_by_long_name(printer);
      if (!the_printer)
	{
	  int i;
	  fprintf(stderr, "Unknown printer %s\nValid printers are:\n",printer);
	  for (i = 0; i < stp_known_printers(); i++)
	    {
	      the_printer = stp_get_printer_by_index(i);
	      fprintf(stderr, "%-16s%s\n", stp_printer_get_driver(the_printer),
		      stp_printer_get_long_name(the_printer));
	    }
	  return 1;
	}
    }
  stp_set_outfunc(v, writefunc);
  stp_set_errfunc(v, writefunc);
  stp_set_outdata(v, stdout);
  stp_set_errdata(v, stderr);
  printfuncs = stp_printer_get_printfuncs(the_printer);
  defparms = printfuncs->default_parameters;
  stp_set_density(v, density);
  if (resolution)
    stp_set_resolution(v, resolution);
  else
    stp_set_resolution(v, (*defparms) (the_printer, NULL, "Resolution"));
  if (ink_type)
    stp_set_ink_type(v, ink_type);
  else
    stp_set_ink_type(v, (*defparms) (the_printer, NULL, "InkType"));
  if (media_type)
    stp_set_media_type(v, media_type);
  else
    stp_set_media_type(v, (*defparms) (the_printer, NULL, "MediaType"));
  if (media_source)
    stp_set_media_source(v, media_source);
  else
    stp_set_media_source(v, (*defparms) (the_printer, NULL, "InputSlot"));
  if (media_size)
    stp_set_media_size(v, media_size);
  else
    stp_set_media_size(v, (*defparms) (the_printer, NULL, "PageSize"));
  if (dither_algorithm)
    stp_set_dither_algorithm(v, dither_algorithm);
  else
    stp_set_dither_algorithm(v, stp_default_dither_algorithm());
  stp_set_driver(v, printer);

  stp_set_output_type(v, OUTPUT_RAW_CMYK);

  (printfuncs->imageable_area)(the_printer, v, &left, &right, &bottom, &top);

  width = right - left;
  height = top - bottom;
  (printfuncs->describe_resolution)(the_printer, stp_get_resolution(v),&x, &y);
  if (levels > width)
    levels = width;

  owidth = width;
  oheight = height;
  width = (width / levels) * levels;
  height = (height / n_fillfuncs) * n_fillfuncs;
  printer_width = width * x / 72;
  printer_height = height * y / 72;

  bandheight = printer_height / n_fillfuncs;
  stp_set_page_width(v, owidth);
  stp_set_page_height(v, oheight);
  stp_set_left(v, ((owidth - width) / 2));
  stp_set_top(v, 0);
  stp_set_orientation(v, ORIENT_PORTRAIT);

  stp_merge_printvars(v, stp_printer_get_printvars(the_printer));
  if (stp_printer_get_printfuncs(the_printer)->verify(the_printer, v))
    (stp_printer_get_printfuncs(the_printer)->print)(the_printer, &theImage, v);
  else
    return 1;
  return 0;
}

/*
  Patterns printed:

  0) White band
  1) C sweep
  2) M sweep
  3) Y sweep
  4) Pure CMY sweep
  5) K sweep
  6) Adjusted CMY sweep
  7) CMYK, transition band 10-30%
  8) Adjusted CMYK, transition band 10-30%
  9) CMYK, transition band 30-70%
 10) Adjusted CMYK, transition band 30-70%
 11) CMYK, transition band 10-99.9%
 12) Adjusted CMYK, transition band 10-99.9%
 13) CMYK, transition band 30-99.9%
 14) Adjusted CMYK, transition band 30-99.9%
 15) CMYK, transition band 50-99.9%
 16) Adjusted CMYK, transition band 50-99.9%
 17) Y+M      (R) sweep
 18) Y+M+.25C     sweep
 19) Y+M+.25C     sweep (using K)
 20) Y+M+.5C      sweep
 21) Y+M+.5C      sweep (using K)
 22) Y+M+.75C     sweep
 23) Y+M+.75C     sweep (using K)
 24) Y+M+.9C      sweep
 25) Y+M+.9C      sweep (using K)
 26) C+Y      (G) sweep
 27) C+Y+.25M     sweep
 28) C+Y+.25M     sweep (using K)
 29) C+Y+.5M      sweep
 30) C+Y+.5M      sweep (using K)
 31) C+Y+.75M     sweep
 32) C+Y+.75M     sweep (using K)
 33) C+Y+.9M      sweep
 34) C+Y+.9M      sweep (using K)
 35) C+M      (B) sweep
 36) C+M+.25Y     sweep
 37) C+M+.25Y     sweep (using K)
 38) C+M+.5Y      sweep
 39) C+M+.5Y      sweep (using K)
 40) C+M+.75Y     sweep
 41) C+M+.75Y     sweep (using K)
 42) C+M+.9Y      sweep
 43) C+M+.9Y      sweep (using K)
 44) White band
*/

typedef void (*fillfunc_t)(unsigned short *data, size_t len, size_t scount);

static void
fill_white(unsigned short *data, size_t len, size_t scount)
{
}

static void
fill_black(unsigned short *data, size_t len, size_t scount)
{
  int i;
  for (i = 0; i < len; i++)
    {
      data[3] = ink_limit;
      data += 4;
    }
}

static void
fill_colors(unsigned short *data, size_t len, size_t scount,
	    double c, double m, double y, double k,
	    double c_level, double m_level, double y_level,
	    double lower, double upper)
{
  int i;
  int j;
  int pixels;
  if (scount > len)
    scount = len;
  pixels = len / scount;
  for (i = 0; i < scount; i++)
    {
      double where = (double) i / ((double) scount - 1);
      double cmyv;
      double kv;
      double val = ink_limit * where;
      double cc = val * c;
      double mm = val * m;
      double yy = val * y;
      double kk = ink_limit * k;
      if (where <= lower)
	{
	  kv = 0;
	}
      else if (where > upper)
	{
	  kv = where;
	}
      else
	{
	  kv = (where - lower) * upper / (upper - lower);
	}
      cmyv = k * (where - kv);
      kk *= kv;
      cc += cmyv * ink_limit * c_level;
      mm += cmyv * ink_limit * m_level;
      yy += cmyv * ink_limit * y_level;
      if (cc > 65535)
	cc = 65535;
      if (mm > 65535)
	mm = 65535;
      if (yy > 65535)
	yy = 65535;
      if (kk > 65535)
	kk = 65535;
      for (j = 0; j < pixels; j++)
	{
	  data[0] = cc;
	  data[1] = mm;
	  data[2] = yy;
	  data[3] = kk;
	  data += 4;
	}
    }
}
  

static void
fill_c(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 1, 0, 0, 0, 1, 1, 1, 1, 1);
}

static void
fill_m(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 1, 0, 0, 1, 1, 1, 1, 1);
}

static void
fill_y(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 1, 0, 1, 1, 1, 1, 1);
}


static void
fill_k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, 1, 1, 1, 0, 0);
}

#if 0
static void
fill_cmy(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 1, 1, 1, 0, 1, 1, 1, 1, 1);
}

static void
fill_cmya(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, c_level, m_level, y_level, 0, 1, 1, 1, 1, 1);
}
#else
static void
fill_cmy(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, 1, 1, 1, 1, 1);
}

static void
fill_cmya(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, c_level, m_level, y_level, 1, 1);
}
#endif

static void
fill_cmyk10_30(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, 1, 1, 1, .1, .3);
}

static void
fill_cmyk30_70a(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, c_level, m_level, y_level, .3, .7);
}

static void
fill_cmyk10_999(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, 1, 1, 1, .1, .999);
}

static void
fill_cmyk30_999(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, 1, 1, 1, .3, .999);
}

static void
fill_cmyk50_999(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, 1, 1, 1, .5, .999);
}

static void
fill_cmyk10_30a(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, c_level, m_level, y_level, .1, .3);
}

static void
fill_cmyk30_70(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, 1, 1, 1, .3, .7);
}

static void
fill_cmyk10_999a(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, c_level, m_level, y_level, .1, .999);
}

static void
fill_cmyk30_999a(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, c_level, m_level, y_level, .3, .999);
}

static void
fill_cmyk50_999a(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 0, 0, 1, c_level, m_level, y_level, .5, .999);
}

static void
fill_ym(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, 1, 1, 0, 1, 1, 1, 1, 1);
}

static void
fill_ym25c(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .75, .75, .25, 1, 1, 1, 1, 1);
}

static void
fill_ym25k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .75, .75, .25, 1, 1, 1, 0, 0);
}

static void
fill_ym5c(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .5, .5, .5, 1, 1, 1, 1, 1);
}

static void
fill_ym5k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .5, .5, .5, 1, 1, 1, 0, 0);
}

static void
fill_ym75c(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .25, .25, .75, 1, 1, 1, 1, 1);
}

static void
fill_ym75k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .25, .25, .75, 1, 1, 1, 0, 0);
}

static void
fill_ym9c(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .1, .1, .9, 1, 1, 1, 1, 1);
}

static void
fill_ym9k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 0, .1, .1, .9, 1, 1, 1, 0, 0);
}


static void
fill_cy(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 1, 0, 1, 0, 1, 1, 1, 1, 1);
}

static void
fill_cy25m(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .75, .0, .75, .25, 1, 1, 1, 1, 1);
}

static void
fill_cy25k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .75, .0, .75, .25, 1, 1, 1, 0, 0);
}

static void
fill_cy5m(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .5, .0, .5, .5, 1, 1, 1, 1, 1);
}

static void
fill_cy5k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .5, .0, .5, .5, 1, 1, 1, 0, 0);
}

static void
fill_cy75m(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .25, .0, .25, .75, 1, 1, 1, 1, 1);
}

static void
fill_cy75k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .25, .0, .25, .75, 1, 1, 1, 0, 0);
}

static void
fill_cy9m(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .1, .0, .1, .9, 1, 1, 1, 1, 1);
}

static void
fill_cy9k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .1, .0, .1, .9, 1, 1, 1, 0, 0);
}

static void
fill_cm(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, 1, 1, 0, 0, 1, 1, 1, 1, 1);
}

static void
fill_cm25y(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .75, .75, 0, .25, 1, 1, 1, 1, 1);
}

static void
fill_cm25k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .75, .75, 0, .25, 1, 1, 1, 0, 0);
}

static void
fill_cm5y(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .5, .5, 0, .5, 1, 1, 1, 1, 1);
}

static void
fill_cm5k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .5, .5, 0, .5, 1, 1, 1, 0, 0);
}

static void
fill_cm75y(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .25, .25, 0, .75, 1, 1, 1, 1, 1);
}

static void
fill_cm75k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .25, .25, 0, .75, 1, 1, 1, 0, 0);
}

static void
fill_cm9y(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .1, .1, 0, .9, 1, 1, 1, 1, 1);
}

static void
fill_cm9k(unsigned short *data, size_t len, size_t scount)
{
  fill_colors(data, len, scount, .1, .1, 0, .9, 1, 1, 1, 0, 0);
}

fillfunc_t the_fillfuncs[] =
{
  fill_white,
  fill_c,
  fill_m,
  fill_y,
  fill_cmy,
  fill_k,
  fill_cmya,
  fill_cmyk10_30,
  fill_cmyk10_30a,
  fill_cmyk30_70,
  fill_cmyk30_70a,
  fill_cmyk10_999,
  fill_cmyk10_999a,
  fill_cmyk30_999,
  fill_cmyk30_999a,
  fill_cmyk50_999,
  fill_cmyk50_999a,
  fill_ym,
  fill_ym25c,
  fill_ym25k,
  fill_ym5c,
  fill_ym5k,
  fill_ym75c,
  fill_ym75k,
  fill_ym9c,
  fill_ym9k,
  fill_cy,
  fill_cy25m,
  fill_cy25k,
  fill_cy5m,
  fill_cy5k,
  fill_cy75m,
  fill_cy75k,
  fill_cy9m,
  fill_cy9k,
  fill_cm,
  fill_cm25y,
  fill_cm25k,
  fill_cm5y,
  fill_cm5k,
  fill_cm75y,
  fill_cm75k,
  fill_cm9y,
  fill_cm9k
};

const int n_fillfuncs = sizeof(the_fillfuncs) / sizeof(fillfunc_t);


static stp_image_status_t
Image_get_row(stp_image_t *image, unsigned char *data, int row)
{
  static int previous_band = -1;
  int band = row / bandheight;
  if (previous_band == -2)
    {
      memset(data, 0, printer_width * 4 * sizeof(unsigned short));
      (the_fillfuncs[band])((unsigned short *)data, printer_width, levels);
      previous_band = band;
    }
  else if (row == printer_height - 1)
    {
      memset(data, 0, printer_width * 4 * sizeof(unsigned short));
      fill_black((unsigned short *)data, printer_width, levels);
    }
  else if (band >= n_fillfuncs)
    memset(data, 0, printer_width * 4 * sizeof(unsigned short));
  else if (band != previous_band && band > 0)
    {
      memset(data, 0, printer_width * 4 * sizeof(unsigned short));
      fill_black((unsigned short *)data, printer_width, levels);
      previous_band = -2;
    }
  return STP_IMAGE_OK;
}

static int
Image_bpp(stp_image_t *image)
{
  return 8;
}

static int
Image_width(stp_image_t *image)
{
  return printer_width;
}

static int
Image_height(stp_image_t *image)
{
  return printer_height;
}

static void
Image_rotate_ccw(stp_image_t *image)
{
 /* dummy function, Landscape printing unsupported atm */
}

static void
Image_rotate_cw(stp_image_t *image)
{
 /* dummy function, Seascape printing unsupported atm */
}

static void
Image_rotate_180(stp_image_t *image)
{
 /* dummy function,  upside down printing unsupported atm */
}

static void
Image_init(stp_image_t *image)
{
 /* dummy function */
}

static void
Image_progress_init(stp_image_t *image)
{
 /* dummy function */
}

/* progress display */
static void
Image_note_progress(stp_image_t *image, double current, double total)
{
  fprintf(stderr, ".");
}

static void
Image_progress_conclude(stp_image_t *image)
{
  fprintf(stderr, "\n");
}

static const char *
Image_get_appname(stp_image_t *image)
{
  return "Test Pattern";
}
