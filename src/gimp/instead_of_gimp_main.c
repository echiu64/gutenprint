/*
 * instead_of_gimp_main.c creates a stand-alone, executable version of gimp-print GUI.
 *
 * This file changes gimp-print so that it is NOT a plug-in for gimp. 
 * This set of functions replace the gimp.c gimp_main() and a host
 * of other functions so that you can send images to gimp-print GUI
 * without having to use gimp. 
 *
 * I intend to use Gtk drawable screens as my source of image data. 
 * I have an application which draws on such a drawable.  With this, 
 * instead_of_gimp_main.c file, I can link gimp-print to my application and use 
 * it as my print dialog. To do this, simply replace main() with your own print 
 * function name and link this in with your application and all the 
 * gimp/print.c, et cetera, functions or you can call it as a separate executable
 * and use arguments to name the image filename and height and width. 
 * If you have a different source of image data other than Gtk, then this project 
 * will be a good starting point to adapt your images for use with gimp-print GUI.
 *
 * My tactic is to imitate all the gimp_*...() functions in this file so the
 * print.c and other gimp-print plug-in files do not have to be changed.
 * That way the same code can serve both the plug-in and this non-plug-in,
 * stand-alone, executable version of gimp-print GUI.
 * The most important gimp_*...()s which access the image are in print-image-gimp.c.
 * (For example, gimp_pixel_rgn_get_row()).  I will imitate all those here too.
 *
 * David Pace, November 25, 2002.
 *
 * After further thought, it occurred to me that a better thing to do is
 * to write my own version of print-image-gimp.c rather than imitate
 * all the gimp_*() functions.  But, this would make my work incompatible
 * with future versions of gimp-print GUI.  So, I guess I will stick with
 * my plan A and write all gimp_*() functions.  Someone in the future might 
 * do plan B. I write this so the reader has a good hint at how this all works.
 * It is more than I got from the current documentation. :-)
 * DPace Nov, 27, 2002.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../../lib/libprintut.h"

#include "print_gimp.h"
/*#include "/home/dpace/gimp-1.2.2/libgimp/gimpdialog.h"*/

#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "print-intl.h"

#ifdef NO_NOT_HERE_SEE_PRINTC
/* print.c calls MAIN() and MAIN() is #defined in gimp.h
 * so I have to redefine MAIN() so that it does nothing
 */
#define MAIN()    \
 void donothing() \
 {                \
 }
#endif

/* This main() can be changed to another function name which you
 * call to do printing in your own application.  I am leaving it
 * as main() so that it can be compiled to create a standalone executable.
 */
void runit( gchar *, gdouble, gdouble, char *, int *, GimpParam **);
void prepare_gimp_drawable_get( GimpDrawable *drawable );
FILE *RGB_image;

#define STANDALONE
#ifdef STANDALONE
int main( int argc, char *argv[])
#else
int gimpprint_panel( FILE *fpgimpprint, unsigned height, unsigned width )
#endif
{                            
  int nreturn_vals;       
  GimpParam retval, *return_vals;   
  GimpDrawable drawable;
  gint32 image_ID = 0, drawable_ID = 0;
  gchar *image_filename;

#ifdef STANDALONE
  gdouble width, height;
  FILE *fpgimpprint;

  gtk_set_locale();
  gtk_init( &argc, &argv);

  if( argc < 4 )
  {
     puts("Usage: gimpprint_gui filename height width (enter)");
     puts("   or  gimpprint_gui filename height width 1(enter) to delete the file.");
     exit(1);
  }

  fpgimpprint = fopen( argv[1], "r" );
  height = atof( argv[2]);
  width = atof( argv[3]);
#endif

  /* Do three things to set up your image */

     /* One */
     /* You name the image here. It will appear as the title of the GUI gtk frame. */
     image_filename = "Rapid Chart";

     /* Two */
     /* drawable holds some dimensions. */
     drawable.height = height; /* you decide */
     drawable.width = width;   /* you decide */

     drawable.id = 1;  /* always 1. It is unused in my version */
     drawable.bpp = 3; /* bytes per pixel is 3 (char) for my RGB Gtk images.
                          Gimp seems to use 1,2,3, or 4 */

     /* I decide all this: */
     drawable.ntile_rows = drawable.width/drawable.bpp; /* I don't use this? # of tile rows*/
     drawable.ntile_cols = drawable.height;    /* I don't use this?  # of tile columns*/
     drawable.tiles = NULL;         /* I don't use this. the normal tiles */
     drawable.shadow_tiles = NULL;  /* I don't use this. the shadow tiles */
     prepare_gimp_drawable_get( &drawable ); /*insert our drawable into gimp_drawable_get()*/
                                         
     /* Three */
     /* Set the static FILE *RGB_image so that all the ...get_row(), ...get_col()
        functions can find the image data. The format is simple for now (Dec/2002).
        But, it could be extended to many image formats.  I am using this format:
        RGBRGBRGB ... one byte per colour, 256 choices per colour, 3 bytes per pixel.
        You must have fpgimpprint = fopen()ed your image file in read mode: "r","r+", or "w+".
        Here are the steps in Gtk/Gdk to grab RGB screen images:
        Boy, I saved you a lot of time by writing this down!
            GdkImage *im;

            im = gdk_image_get( pixm,
                       0, 
                       0,
                       drawarea->allocation.width,
                       drawarea->allocation.height
                     );
            pixrgb = gdk_image_get_pixel( im, xx, yy);
            red = (pixrgb & 0xFF0000) >> 16;
            grn = (pixrgb & 0x00FF00) >> 8;
            blu = (pixrgb & 0x0000FF);
            gdk_image_destroy( im );
      */
     if( fpgimpprint != NULL )
     {
        RGB_image = fpgimpprint;

        return_vals = &retval;             
                                           
        /* send along all parameters even if some are not used */
        runit( image_filename, height, width, "file_print_gimp", &nreturn_vals, &return_vals );    

        if( argc == 5 && *argv[4] == '1' )
           unlink( argv[1] );

        return(1);
     }
     return(0);
}

void
gimp_ui_init (const gchar *prog_name,
	      gboolean     preview)
{
  gint    argc;
  gchar **argv;
  static gboolean initialized = FALSE;

  g_return_if_fail (prog_name != NULL);

  if( initialized == TRUE )
    return;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup (prog_name);

  gtk_init (&argc, &argv);

#ifdef GGG
  gtk_rc_parse (gimp_gtkrc ());

  user_gtkrc = gimp_personal_rc_file ("gtkrc");
  gtk_rc_parse (user_gtkrc);
  g_free (user_gtkrc);

  /*  It's only safe to switch Gdk SHM usage off  */
  if (! gimp_use_xshm ())
#endif
    gdk_set_use_xshm (FALSE);

/*
  gdk_rgb_set_min_colors (gimp_min_colors ());
  gdk_rgb_set_install (gimp_install_cmap ());
*/

  gtk_widget_set_default_visual (gdk_rgb_get_visual ());
  gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());

  /*  Set the gamma after installing the colormap because
   *  gtk_preview_set_gamma() initializes GdkRGB if not already done
   */
/*
  if( preview )
    gtk_preview_set_gamma( 1.0 );   gimp_gamma 
*/

  initialized = TRUE;

  return;
}


static GimpDrawable *thedrawable = NULL, a_drawable;
gint _gimp_tile_width  = -1;
gint _gimp_tile_height = -1;

/* this is my prepare_ function, not gimp's */
void prepare_gimp_drawable_get( GimpDrawable *drawable )
{
   thedrawable = drawable;

   _gimp_tile_width  = drawable->ntile_rows;
   _gimp_tile_height = drawable->ntile_cols;

   return;
}

guint
gimp_tile_width (void)
{
  return _gimp_tile_width;
}

GimpDrawable *drawable_get()
{
  /* ignore the drawable_ID and just talk about our own image here */

  if( thedrawable == NULL )
  {
     /* This should never happen
        if you first called prepare_gimp_drawable_get(drawable )
        to load up your image properly as shown in main() 
      */

     a_drawable.id = 1;
     a_drawable.width = 100;
     a_drawable.height = 100;
     a_drawable.bpp = 3;
     a_drawable.ntile_rows = a_drawable.width/a_drawable.bpp;    /* # of tile columns */
     a_drawable.ntile_cols = a_drawable.height;    /* # of tile rows */
     a_drawable.tiles = NULL;         /* the normal tiles */
     a_drawable.shadow_tiles = NULL;  /* the shadow tiles */

     return( &a_drawable );
  }
     
  return( thedrawable );
}


/** Do nothing */
gboolean
gimp_progress_init (gchar *message)
{
  return TRUE;
}

/** Do nothing */
gboolean
gimp_progress_update (gdouble percentage)
{

  return TRUE;
}

/* this is what Image_GimpDrawable_new() does:
 *
 * See:
   stp_image_t theImage; inside print-image-gimp.c

 *   stp_image_t *
 *   Image_GimpDrawable_new(GimpDrawable *drawable)
 *   {
 *      Gimp_Image_t *i = xmalloc(sizeof(Gimp_Image_t));
 *      i->drawable = drawable;
 *      gimp_pixel_rgn_init(&(i->rgn), drawable, 0, 0,
 *                        drawable->width, drawable->height, FALSE, FALSE);
 *      theImage.rep = i;
 *      theImage.reset(&theImage);
 *
 *      return &theImage;
 *   }
 * So, I must imitate gimp_pixel_rgn_init() and gimp_pixel_rgn_get_row() ... .
 * These gimp_*() functions are the most important functions to imitate because
 * it is the real source from which the image is being printed.
 * Here is the structure of stp_image_t (from gimp-print.h)

    typedef struct stp_image
    {
      void (*init)(struct stp_image *image);
      void (*reset)(struct stp_image *image);
      int  (*bpp)(struct stp_image *image);
      int  (*width)(struct stp_image *image);
      int  (*height)(struct stp_image *image);
      stp_image_status_t (*get_row)(struct stp_image *image, unsigned char *data,
                                    int row);
      const char *(*get_appname)(struct stp_image *image);
      void (*progress_init)(struct stp_image *image);
      void (*note_progress)(struct stp_image *image, double current, double total);
      void (*progress_conclude)(struct stp_image *image);
      void *rep;
    } stp_image_t;

   Apparently, I have to create a function which imitates each of these activities which
   are inside this structure. Functions are already nicely assigned to each function pointer
   inside print-image-gimp.c. (*get_row)(), for example, points at something
   that calls gimp_pixel_rgn_get_row() in print-image-gimp.c.
*/

#define TILE_WIDTH 64
#define TILE_HEIGHT 64

/* fill the rgn structure with values. I must set drawable to correct values first. */
void gimp_pixel_rgn_init( GimpPixelRgn *rgn,
                          GimpDrawable *drawable,
                          gint          xx,
                          gint          yy,
                          gint          width,
                          gint          height,
                          gboolean      dirty,
                          gboolean      shadow )
{
  rgn->data      = NULL;
  rgn->drawable  = drawable;
  rgn->bpp       = drawable->bpp;
  rgn->rowstride = rgn->bpp * TILE_WIDTH;
  rgn->x         = xx;
  rgn->y         = yy;
  rgn->w         = width;
  rgn->h         = height;
  rgn->dirty     = dirty;
  rgn->shadow    = shadow;

  return;
}


void gimp_pixel_rgn_get_row( GimpPixelRgn *rgn,
			     guchar *buf,
			     gint   xx,
			     gint   yy,
			     gint   width )
{
  gint bpp;
  gint end;

  end = xx + width;

  /*printf("\ninside get_row xx=%d,yy=%d,end=%d, width=%d\n",xx,yy,end,width);fflush(stdout);*/

  bpp = rgn->drawable->bpp;

  fseek( RGB_image, bpp * yy * rgn->drawable->width, SEEK_SET); 

  fread( buf, bpp * width, sizeof(char), RGB_image);

  return;
}

void gimp_pixel_rgn_get_col( GimpPixelRgn *rgn,
			guchar       *buf,
			gint          xx,
			gint          yy,
			gint          height)
{
  gint end;
  gint bpp;

  /*printf("\nget_col xx=%d, yy=%d", xx,yy ); fflush(stdout);*/

  end = yy + height;
  bpp = rgn->drawable->bpp;

  for( ; yy < end; yy++)
  {
     /* move to the right xx and down yy rows */
     fseek( RGB_image, bpp * (xx + yy*rgn->drawable->width), SEEK_SET); 
     if( fread( buf, bpp, sizeof(char), RGB_image) != bpp )
        break;
     buf += bpp;
  }

  return;
}


/*
 * gimp_image_get_cmap:
 * @image_ID: The image.
 * @num_colors: Number of colors in the colormap array.
 *
 * Returns the image's colormap
 *
 * This procedure returns an actual pointer to the image's colormap, as
 * well as the number of colors contained in the colormap. If the image
 * is not of base type INDEXED, this pointer will be NULL.
 *
 * Returns: The image's colormap.
 */
guchar *gimp_image_get_cmap (gint32  image_ID, gint   *num_colors)
{
  gint    num_bytes;
  guchar *cmap;

  cmap = NULL;  /*_gimp_image_get_cmap (image_ID, &num_bytes);*/

  num_bytes = 3;
  *num_colors = num_bytes;

  return cmap;
}

static gulong      max_cache_size  = 0;

void
gimp_tile_cache_size (gulong kilobytes)
{
  max_cache_size = kilobytes * 1024;
}

void
gimp_tile_cache_ntiles (gulong ntiles)
{
  gimp_tile_cache_size ((gulong)(ntiles * _gimp_tile_width * _gimp_tile_height * 4 + 1023) / 1024);
}

GimpImageBaseType gimp_image_base_type( gint32 image_ID)
{
  return( GIMP_RGB );
}



/**
 * gimp_personal_rc_file:
 * @basename: The basename of a rc_file.
 *
 * Returns the name of a file in the user-specific GIMP settings directory.
 *
 * The returned string is allocated dynamically and *SHOULD* be freed
 * with g_free() after use.
 *
 * Returns: The name of a file in the user-specific GIMP settings directory.
 **/
gchar*
gimp_personal_rc_file( gchar *basename)
{
  printf( "%s (I wonder what happens if this directory does not exist)\nAlso, I need to clean up the code.\nBut, basically this first version works, dpace Jan/2003.\n",
          gimp_directory() 
        );
  fflush(stdout);

  return g_strconcat( gimp_directory(),
		      G_DIR_SEPARATOR_S,
		      basename,
		      NULL);
}


/**
 * gimp_directory:
 *
 * Returns the user-specific GIMP settings directory. If the environment 
 * variable GIMP_DIRECTORY exists, it is used. If it is an absolute path, 
 * it is used as is.  If it is a relative path, it is taken to be a 
 * subdirectory of the home directory. If it is relative path, and no home 
 * directory can be determined, it is taken to be a subdirectory of
 * gimp_data_directory().
 *
 * The usual case is that no GIMP_DIRECTORY environment variable exists, 
 * and then we use the GIMPDIR subdirectory of the home directory. If no 
 * home directory exists, we use a per-user subdirectory of
 * gimp_data_directory().
 * In any case, we always return some non-empty string, whether it
 * corresponds to an existing directory or not.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The user-specific GIMP settings directory.
 **/
gchar*
gimp_directory (void)
{
return( (gchar *)"/home/dpace/.gimp-1.2/" );
}

/*******************************************************
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../../lib/libprintut.h"

#include <gimp-print/gimp-print-ui.h>
#include "print_gimp.h"

#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "print-intl.h"
********************************************************/

static int	do_print_dialog (char *proc_name);

/*
 * Work around GIMP library not being const-safe.  This is a very ugly
 * hack, but the excessive warnings generated can mask more serious
 * problems.
 */

#define BAD_CONST_CHAR char *

/*
 * Globals...
 */

static stpui_plist_t gimp_vars;

static gint32    image_ID_printc;   /* image ID */


static guchar *
stpui_get_thumbnail_data_function(void *image_ID_printc, 
                                  gint *ww, 
                                  gint *hh,
				  gint *bpp, 
                                  gint page)
{
  /* gimp's version seems to call image_thumbnail_invoker(); in gimp.../app/  */

  guchar *buf, *bufstart=NULL, clr;
  gint end;
  gint bb, xx, yy;
  gboolean success = TRUE;
  GimpDrawable *drawable;
  gint req_width, req_height;
  gfloat incw, inc_h;
  gint iiw, iih, imitatew, imitateh;
  gint dwidth, dheight;

  drawable =  drawable_get();

  /* my version discards image_ID_printc */

  /* some of this is from image_thumbnail_invoker(), gimp.../app/image_cmds.c */
  if( *ww <= 0)
    success = FALSE;

  if( *hh <= 0)
    success = FALSE;

  if(success == TRUE)
  {
    req_width = *ww; 
    req_height = *hh; 

    /* I grabbed this part from thumbnail_invoker(). I'm not sure if I need this. */
    if( req_width <= 128 && req_height <= 128)
    {
        /* Adjust the width/height ratio */
        dwidth = drawable->width;
        dheight = drawable->height;
 
        if( dwidth > dheight)
          req_height = (req_width * dheight) / dwidth;
        else
          req_width = (req_height * dwidth) / dheight;

        *ww = req_width;
        *hh = req_height;
    }

    *bpp = drawable->bpp;   /* we set bpp. It isn't telling us. */
    end = ( *ww * *hh );
    bufstart = buf = malloc( *bpp * end );
 
 /*printf("\ninside thumbnail ww=%d,hh=%d,end=%d, bpp=%d\n", *ww,*hh,end,*bpp ); fflush(stdout);*/
 
    if( buf == NULL )
    {
       puts("malloc failed in ...thumbnail_data()");
       exit(1);
    }
    else
    {
      fseek( RGB_image, 0, SEEK_SET ); /* start of image */
 
      incw = (float)drawable->width / (float)(*ww);
      inc_h = (float)drawable->height / (float)(*hh);
 
      for( iih=yy=iiw=xx=0; xx < end; )
      {
	 for(bb = 0; bb < *bpp; bb++)
         {
            if( fread( buf, 1, sizeof(char), RGB_image) == 0 )
               break;
            ++buf;
         }
 
         ++xx;
         ++iiw;
 
         /* calc how far across drawable->width we should be if not for *ww less than width */
         /* ie. a thumbnail is small so skip some pixels */
         imitatew = (float)xx * incw;
         for( ; iiw < imitatew; ++iiw )
         {
              /* fseek( RGB_image, *bpp, SEEK_CUR );  throw away extra pixels */
            
              buf -= (*bpp);
              for( bb=0; bb < *bpp; ++bb, ++buf )
              { 
                /* average together extra pixels instead of dumping them */
                /* to get a more detailed image */
 
                if( fread( &clr, 1, sizeof(char), RGB_image) == 1 )  
                { 
                   *buf = (guchar)( ( (unsigned)(*buf) + (unsigned)(clr) ) / 2);
                }
              }
         }
 
         if( (xx % (*ww)) == 0 ) /* have we moved to a new line yet?*/ 
         {
            /* what about skipping extra height lines? */
 
            yy = xx / (*ww); /* we are on line number yy */
 
            /* calc how far down drawable->height we should be 
               if not for *hh less than height */
 
            /* ie. a thumbnail is small so skip some lines */
            imitateh = (gfloat)yy * inc_h;
            for( ; iih < imitateh; ++iih )
               fseek( RGB_image, (*bpp)*(drawable->width), SEEK_CUR );/*throw extra lines*/
         }
      }
    }
  }
  return(bufstart);
}

void
runit(gchar *image_filename, 
      gdouble height, 
      gdouble width, 
      char   *name,		/* I - Name of print program. */
      int    *nreturn_vals,	/* O - Number of return values */
      GimpParam **return_vals)	/* O - Return values */
{
  GimpDrawable	*drawable;	/* Drawable for image */
  GimpParam	*values;	/* Return values */
  gint32         drawable_ID;   /* drawable ID */
  stp_image_t *image;

 /*
  * Initialise libgimpprint
  */

  stp_init();

#ifdef INIT_I18N_UI
  INIT_I18N_UI();
#else
  /*
   * With GCC and glib 1.2, there will be a warning here about braces in
   * expressions.  Getting rid of it causes more problems than it solves.
   * In particular, turning on -ansi on the command line causes a number of
   * other useful things, such as strcasecmp, popen, and snprintf to go away
   */
  INIT_LOCALE (PACKAGE);
#endif

  stpui_printer_initialize(&gimp_vars);
  stp_set_input_color_model(gimp_vars.v, COLOR_MODEL_RGB);
  /*
   * Initialize parameter data...
   */

  values = g_new (GimpParam, 1);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  if( strchr(image_filename, '/'))
    image_filename = strrchr(image_filename, '/') + 1;
  stpui_set_image_filename(image_filename);

  gimp_ui_init ("print", TRUE);

  drawable = drawable_get();
  stpui_set_image_dimensions( (guint)width, (guint)height );
  stpui_set_image_resolution( width, height);
  image = Image_GimpDrawable_new(drawable, image_ID_printc);
  stp_set_float_parameter(gimp_vars.v, "AppGamma", 1.0 );  /* 1.0 gimp_gamma */

  /*
   * Get information from the dialog...
   */
   
  if( do_print_dialog( name) )
  {
     stpui_plist_copy(&gimp_vars, stpui_get_current_printer());
   
     /*
      * Print the image...
      */
     if (values[0].data.d_status == GIMP_PDB_SUCCESS)
     {
         /*
          * Set the tile cache size...
          */
   
         if( height > width)
	   gimp_tile_cache_ntiles (( (guint)height + gimp_tile_width () - 1) /
				   gimp_tile_width () + 1);
         else
	   gimp_tile_cache_ntiles (( (guint)width + gimp_tile_width () - 1) /
				   gimp_tile_width () + 1);
   
         if (! stpui_print(&gimp_vars, image))
	     values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
   
         /*
          * Store data...
          * FIXME! This is broken!
          */
   
#if 0
      gimp_set_data (PLUG_IN_NAME, vars, sizeof (vars));
#endif
     }
   
  }
  stp_vars_free(gimp_vars.v);
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

static void
gimp_writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

static gint
do_print_dialog (gchar *proc_name)
{
 /*
  * Generate the filename for the current user...
  */
  char *filename = gimp_personal_rc_file ((BAD_CONST_CHAR) "printrc");
  stpui_set_printrc_file(filename);
  g_free(filename);
  stpui_set_errfunc(gimp_writefunc);
  stpui_set_errdata(stderr);
  stpui_set_thumbnail_func(stpui_get_thumbnail_data_function);
  stpui_set_thumbnail_data((void *) image_ID_printc);
  return stpui_do_print_dialog();
}

