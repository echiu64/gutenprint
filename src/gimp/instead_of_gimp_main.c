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

static GtkTooltips *tool_tips  = NULL;

/* This main() can be changed to another function name which you
 * call to do printing in your own application.  I am leaving it
 * as main() so that it can be compiled to create a standalone executable.
 */
int gimp_print_gui( FILE *, unsigned, unsigned );
void runit( char *, int *, GimpParam **);
void prepare_gimp_drawable_get( GimpDrawable *drawable );
FILE *RGB_image;

#define STANDALONE
#ifdef STANDALONE
int main( int argc, char *argv[])
#else
int gimp_print_gui( FILE *fpgimpprint, unsigned height, unsigned width )
#endif
{                            
  int nreturn_vals;       
  GimpParam retval, *return_vals;   
  GimpDrawable drawable;
  gint32 image_ID = 0, drawable_ID = 0;

#ifdef STANDALONE
  unsigned width, height;
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
  height = atoi( argv[2]);
  width = atoi( argv[3]);
#endif

  tool_tips = gtk_tooltips_new();

  /* Do three things to set up your image */

     /* One */
     /* You name the image here. It will appear as the title of the GUI gtk frame. */
     gimp_image_set_filename( image_ID, "Rapid Chart" ); /* image_ID does nothing */

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
        runit( "file_print_gimp", &nreturn_vals, &return_vals );    

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
    gtk_preview_set_gamma( gimp_gamma() );
*/

  initialized = TRUE;
}


/* must imitate gimp_image_set_filename() and ...get...() as in the real gimp */
static gchar *thefilename = NULL;

gboolean gimp_image_set_filename( gint32 image_ID, gchar *nm ) 
                                       /*image_ID is unused in my version*/
{
  static const char *error = "memory error gimp_image_set_filename()";

  if( thefilename != NULL && thefilename != error )
     free(thefilename);

  thefilename = malloc( strlen(nm) + 1 ); 
  if( thefilename == NULL )
     thefilename = error;
  else
     strcpy( thefilename, nm );

  return(TRUE);
}
gchar *gimp_image_get_filename( gint32 image_ID ) /* image_ID is unused in my version*/
{
  return( thefilename );
}


/* must imitate gimp_image_get_resolution() as in the real gimp */

gboolean gimp_image_get_resolution( gint32 image_ID, gdouble *xres, gdouble *yres)  
                                        /* image_ID is unused */
{
   gboolean ret = TRUE;

   *xres = 800;
   *yres = 559;

   return( ret );
}

/* must imitate gimp_gamma because it is in gimp_ui_init() */
gdouble gimp_gamma()
{
  gdouble ret = 1.0;

  return( ret );
}

/* must imitate gimp_export_image() in real gimp */
GimpExportReturnType 
gimp_export_image( gint32 *image_ID, 
                   gint32 *drawable_ID, 
                   const gchar *Print, 
                   GimpExportCapabilities exp )
{
   GimpExportReturnType export = !GIMP_EXPORT_CANCEL; /* assure NOT cancelling */

   return(export);
}

/* do nothing: */
void gimp_drawable_detach( GimpDrawable *drawable )
{
}

/* do nothing: */
gboolean gimp_image_delete( gint32 image_ID )
{
   gboolean ret = TRUE;

   return( ret );
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


/* must imitate gimp_drawable_get() in real gimp */
GimpDrawable *gimp_drawable_get( gint32 drawable_ID )
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

/* do nothing. used in gimp_main_window.c */
void gimp_help_init()
{
}

/* These tool tip items are the same as in gimp.
 * I put them here so that we don't need to
 * have the gimp library bound in.
 */
void
gimp_help_enable_tooltips (void)
{
  gtk_tooltips_enable(tool_tips);
}

void
gimp_help_disable_tooltips (void)
{
  gtk_tooltips_disable(tool_tips);
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

/* This one is slightly different than gimp's version.
 * But it does the same thing with my return; statements.
 */
void
gimp_help_set_help_data (GtkWidget   *widget,
			 const gchar *tooltip,
			 const gchar *help_data)
{
  if(widget == NULL)
     return;
  if( !GTK_IS_WIDGET(widget) )
     return;

  if( tooltip )
  {
      gtk_tooltips_set_tip (tool_tips, widget, tooltip,
			    (gpointer) help_data);
  }
  else if( help_data )
  {
      gtk_object_set_data( GTK_OBJECT (widget), "gimp_help_data", (gpointer) help_data);
  }
  return;
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

/* must imitate gimp_image_get_thumbnail_data() as in the real gimp*/

guchar *gimp_image_get_thumbnail_data( gint32 image_ID, gint *ww, gint *hh, gint *bpp )
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

  drawable =  gimp_drawable_get(0);

  /* my version discards image_ID */

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

#if 0
/* from gimp-1.2.2/libgimp/gimp.h and print.c*/
#define gimp_set_data         gimp_procedural_db_set_data */

gimp_procedural_db_set_data( PLUG_IN_NAME, vars, sizeof (vars))
{
}
#endif


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

void *xmalloc (size_t size)
{
  register void *memptr = NULL;

  if ((memptr = malloc (size)) == NULL)
  {
      fprintf(stderr, "memory exhausted: xmalloc().\n");
      exit(1);
  }
  return (memptr);
}



GtkWidget * gimp_dialog_new                 (const gchar        *title,
					     const gchar        *wmclass_name,
					     GimpHelpFunc        help_func,
					     const gchar        *help_data,
					     GtkWindowPosition   position,
					     gint                allow_shrink,
					     gint                allow_grow,
					     gint                auto_shrink,

					     /* specify action area buttons
					      * as va_list:
					      *  const gchar    *label,
					      *  GtkSignalFunc   callback,
					      *  gpointer        data,
					      *  GtkObject      *slot_object,
					      *  GtkWidget     **widget_ptr,
					      *  gboolean        default_action,
					      *  gboolean        connect_delete,
					      */

					     ...)
{
  GtkWidget *dialog;
  va_list    args;

  va_start (args, auto_shrink);

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (wmclass_name != NULL, NULL);

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), wmclass_name, "Gimp");
  gtk_window_set_position (GTK_WINDOW (dialog), position);
  gtk_window_set_policy (GTK_WINDOW (dialog), allow_shrink, allow_grow, auto_shrink);

  /*  prepare the action_area  */
  gimp_dialog_create_action_areav (GTK_DIALOG (dialog), args);

  /*  connect the "F1" help key  */
/*
  if (help_func)
    gimp_help_connect_help_accel (dialog, help_func, help_data);
 */
  va_end (args);

  return dialog;
}

/*  local callbacks of gimp_dialog_new ()  */
static gint
gimp_dialog_delete_callback (GtkWidget *widget,
                             GdkEvent  *event,
                             gpointer   data)
{
  GtkSignalFunc  cancel_callback;
  GtkWidget     *cancel_widget;

  cancel_callback =
    (GtkSignalFunc) gtk_object_get_data (GTK_OBJECT (widget),
                                         "gimp_dialog_cancel_callback");
  cancel_widget =
    (GtkWidget*) gtk_object_get_data (GTK_OBJECT (widget),
                                      "gimp_dialog_cancel_widget");

  /*  the cancel callback has to destroy the dialog  */
  if (cancel_callback)
    (* cancel_callback) (cancel_widget, data);

  return TRUE;
}


/**
 * gimp_dialog_create_action_areav:
 * @dialog: The #GtkDialog you want to create the action_area for.
 * @args: A @va_list as obtained with va_start() describing the action_area
 *        buttons.
 *
 */
void
gimp_dialog_create_action_areav (GtkDialog *dialog,
				 va_list    args)
{
  GtkWidget *hbbox = NULL;
  GtkWidget *button;

  /*  action area variables  */
  const gchar    *label;
  GtkSignalFunc   callback;
  gpointer        data;
  GtkObject      *slot_object;
  GtkWidget     **widget_ptr;
  gboolean        default_action;
  gboolean        connect_delete;

  gboolean delete_connected = FALSE;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTK_IS_DIALOG (dialog));

  /*  prepare the action_area  */
  label = va_arg (args, const gchar *);

  if (label)
    {
      gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 2);
      gtk_box_set_homogeneous (GTK_BOX (dialog->action_area), FALSE);

      hbbox = gtk_hbutton_box_new ();
      gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
      gtk_box_pack_end (GTK_BOX (dialog->action_area), hbbox, FALSE, FALSE, 0);
      gtk_widget_show (hbbox);
    }

  /*  the action_area buttons  */
  while (label)
    {
      callback       = va_arg (args, GtkSignalFunc);
      data           = va_arg (args, gpointer);
      slot_object    = va_arg (args, GtkObject *);
      widget_ptr     = va_arg (args, GtkWidget **);
      default_action = va_arg (args, gboolean);
      connect_delete = va_arg (args, gboolean);

      button = gtk_button_new_with_label (label);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

      if (slot_object == (GtkObject *) 1)
	slot_object = GTK_OBJECT (dialog);

      if (data == NULL)
	data = dialog;

      if (callback)
	{
	  if (slot_object)
	    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				       GTK_SIGNAL_FUNC (callback),
				       slot_object);
	  else
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (callback),
				data);
	}

      if (widget_ptr)
	*widget_ptr = button;

      if (connect_delete && callback && !delete_connected)
	{
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp_dialog_cancel_callback",
			       callback);
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp_dialog_cancel_widget",
			       slot_object ? slot_object : GTK_OBJECT (button));

	  /*  catch the WM delete event  */
	  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			      GTK_SIGNAL_FUNC (gimp_dialog_delete_callback),
			      data);

	  delete_connected = TRUE;
	}

      if (default_action)
	gtk_widget_grab_default (button);
      gtk_widget_show (button);

      label = va_arg (args, gchar *);
    }
}

/**
 * gimp_scale_entry_new:
 * @table:               The #GtkTable the widgets will be attached to.
 * @column:              The column to start with.
 * @row:                 The row to attach the widgets.
 * @text:                The text for the #GtkLabel which will appear
 *                       left of the #GtkHScale.
 * @scale_usize:         The minimum horizontal size of the #GtkHScale.
 * @spinbutton_usize:    The minimum horizontal size of the #GtkSpinButton.
 * @value:               The initial value.
 * @lower:               The lower boundary.
 * @upper:               The upper boundary.
 * @step_increment:      The step increment.
 * @page_increment:      The page increment.
 * @digits:              The number of decimal digits.
 * @constrain:           #TRUE if the range of possible values of the
 *                       #GtkSpinButton should be the same as of the #GtkHScale.
 * @unconstrained_lower: The spinbutton's lower boundary
 *                       if @constrain == #FALSE.
 * @unconstrained_upper: The spinbutton's upper boundary
 *                       if @constrain == #FALSE.
 * @tooltip:             A tooltip message for the scale and the spinbutton.
 * @help_data:           The widgets' help_data (see gimp_help_set_help_data()).
 *
 * This function creates a #GtkLabel, a #GtkHScale and a #GtkSpinButton and
 * attaches them to a 3-column #GtkTable.
 *
 * Note that if you pass a @tooltip or @help_data to this function you'll
 * have to initialize GIMP's help system with gimp_help_init() before using it.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 **/
GtkObject *
gimp_scale_entry_new (GtkTable    *table,
		      gint         column,
		      gint         row,
		      const gchar *text,
		      gint         scale_usize,
		      gint         spinbutton_usize,
		      gfloat       value,
		      gfloat       lower,
		      gfloat       upper,
		      gfloat       step_increment,
		      gfloat       page_increment,
		      guint        digits,
		      gboolean     constrain,
		      gfloat       unconstrained_lower,
		      gfloat       unconstrained_upper,
		      const gchar *tooltip,
		      const gchar *help_data)
{
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkObject *return_adj;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label,
                    column, column + 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

#ifdef SKIP_THIS_BECAUSE_CONSTRAINED_IS_ALWAYS_TRUE_DPACE
  if (! constrain &&
      unconstrained_lower <= lower &&
      unconstrained_upper >= upper)
    {
      GtkObject *constrained_adj;

      constrained_adj = gtk_adjustment_new (value, lower, upper,
					    step_increment, page_increment,
					    0.0);

      spinbutton = gimp_spin_button_new (&adjustment, value,
					 unconstrained_lower,
					 unconstrained_upper,
					 step_increment, page_increment, 0.0,
					 1.0, digits);

      gtk_signal_connect
	(GTK_OBJECT (constrained_adj), "value_changed",
	 GTK_SIGNAL_FUNC (gimp_scale_entry_unconstrained_adjustment_callback),
	 adjustment);

      gtk_signal_connect
	(GTK_OBJECT (adjustment), "value_changed",
	 GTK_SIGNAL_FUNC (gimp_scale_entry_unconstrained_adjustment_callback),
	 constrained_adj);

      return_adj = adjustment;

      adjustment = constrained_adj;
    }
  else
    {
#endif
      spinbutton = gimp_spin_button_new (&adjustment, value, lower, upper,
					 step_increment, page_increment, 0.0,
					 1.0, digits);

      return_adj = adjustment;
#ifdef SKIP_THIS_BECAUSE_CONSTRAINED_IS_ALWAYS_TRUE_DPACE
    }
#endif
    
  if (spinbutton_usize > 0)
    gtk_widget_set_usize (spinbutton, spinbutton_usize, -1);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  if (scale_usize > 0)
    gtk_widget_set_usize (scale, scale_usize, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale,
		    column + 1, column + 2, row, row + 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  gtk_table_attach (GTK_TABLE (table), spinbutton,
		    column + 2, column + 3, row, row + 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinbutton);

  if (tooltip || help_data)
    {
      gimp_help_set_help_data (scale, tooltip, help_data);
      gimp_help_set_help_data (spinbutton, tooltip, help_data);
    }

  gtk_object_set_data (GTK_OBJECT (return_adj), "label", label);
  gtk_object_set_data (GTK_OBJECT (return_adj), "scale", scale);
  gtk_object_set_data (GTK_OBJECT (return_adj), "spinbutton", spinbutton);

  return return_adj;
}

/**
 * gimp_spin_button_new:
 * @adjustment:     Returns the spinbutton's #GtkAdjustment.
 * @value:          The initial value of the spinbutton.
 * @lower:          The lower boundary.
 * @upper:          The uppper boundary.
 * @step_increment: The spinbutton's step increment.
 * @page_increment: The spinbutton's page increment (mouse button 2).
 * @page_size:      The spinbutton's page size.
 * @climb_rate:     The spinbutton's climb rate.
 * @digits:         The spinbutton's number of decimal digits.
 *
 * This function is a shortcut for gtk_adjustment_new() and a subsequent
 * gtk_spin_button_new() and does some more initialisation stuff like
 * setting a standard minimun horizontal size.
 *
 * Returns: A #GtkSpinbutton and it's #GtkAdjustment.
 **/
GtkWidget *
gimp_spin_button_new (GtkObject **adjustment,  /* return value */
		      gfloat      value,
		      gfloat      lower,
		      gfloat      upper,
		      gfloat      step_increment,
		      gfloat      page_increment,
		      gfloat      page_size,
		      gfloat      climb_rate,
		      guint       digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
				    step_increment, page_increment, page_size);

  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (*adjustment),
				    climb_rate, digits);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, -1);

  return spinbutton;
}

/**
 * gimp_option_menu_new:
 * @menu_only: #TRUE if the function should return a #GtkMenu only.
 * @...:       A #NULL terminated @va_list describing the menu items.
 *
 * Returns: A #GtkOptionMenu or a #GtkMenu (depending on @menu_only).
 **/
GtkWidget *
gimp_option_menu_new (gboolean            menu_only,

		      /* specify menu items as va_list:
		       *  const gchar    *label,
		       *  GtkSignalFunc   callback,
		       *  gpointer        data,
		       *  gpointer        user_data,
		       *  GtkWidget     **widget_ptr,
		       *  gboolean        active
		       */

		       ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  /*  menu item variables  */
  const gchar    *label;
  GtkSignalFunc   callback;
  gpointer        data;
  gpointer        user_data;
  GtkWidget     **widget_ptr;
  gboolean        active;

  va_list args;
  gint    i;
  gint    initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;

  va_start (args, menu_only);
  label = va_arg (args, const gchar *);

  for (i = 0; label; i++)
    {
      callback   = va_arg (args, GtkSignalFunc);
      data       = va_arg (args, gpointer);
      user_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, GtkWidget **);
      active     = va_arg (args, gboolean);

      if (strcmp (label, "---"))
	{
	  menuitem = gtk_menu_item_new_with_label (label);

	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      callback,
			      data);

	  if (user_data)
	    gtk_object_set_user_data (GTK_OBJECT (menuitem), user_data);
	}
      else
	{
	  menuitem = gtk_menu_item_new ();

	  gtk_widget_set_sensitive (menuitem, FALSE);
	}

      gtk_menu_append (GTK_MENU (menu), menuitem);

      if (widget_ptr)
	*widget_ptr = menuitem;

      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (active)
	initial_index = i;

      label = va_arg (args, const gchar *);
    }
  va_end (args);

  if (! menu_only)
    {
      GtkWidget *optionmenu;

      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      /*  select the initial menu item  */
      gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), initial_index);

      return optionmenu;
    }

  return menu;
}

/**
 * gimp_table_attach_aligned:
 * @table:      The #GtkTable the widgets will be attached to.
 * @column:     The column to start with.
 * @row:        The row to attach the eidgets.
 * @label_text: The text for the #GtkLabel which will be attached left of the
 *              widget.
 * @xalign:     The horizontal alignment of the #GtkLabel.
 * @yalign:     The vertival alignment of the #GtkLabel.
 * @widget:     The #GtkWidget to attach right of the label.
 * @colspan:    The number of columns the widget will use.
 * @left_align: #TRUE if the widget should be left-aligned.
 *
 * Note that the @label_text can be #NULL and that the widget will be attached
 * starting at (@column + 1) in this case, too.
 **/
void
gimp_table_attach_aligned (GtkTable    *table,
			   gint         column,
			   gint         row,
			   const gchar *label_text,
			   gfloat       xalign,
			   gfloat       yalign,
			   GtkWidget   *widget,
			   gint         colspan,
			   gboolean     left_align)
{
  if (label_text)
    {
      GtkWidget *label;

      label = gtk_label_new (label_text);
      gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
      gtk_table_attach (table, label,
			column, column + 1,
			row, row + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  if (left_align)
    {
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (alignment), widget);
      gtk_widget_show (widget);

      widget = alignment;
    }

  gtk_table_attach (table, widget,
		    column + 1, column + 1 + colspan,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (widget);
}

/* do nothing - dpace */
/*  The standard help function  */
void
gimp_standard_help_func (const gchar *help_data)
{
  /*gimp_help (NULL, help_data);*/
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

#ifdef DPACE_SKIP_IT
  static gchar *gimp_dir = NULL;
  gchar *env_gimp_dir;
  gchar *home_dir;
  gchar *home_dir_sep;

  if (gimp_dir != NULL)
    return gimp_dir;

  env_gimp_dir = g_getenv ("GIMP_DIRECTORY");
  home_dir = g_get_home_dir ();

  if (home_dir != NULL && home_dir[strlen (home_dir)-1] != G_DIR_SEPARATOR)
    home_dir_sep = G_DIR_SEPARATOR_S;
  else
    home_dir_sep = "";

  if (NULL != env_gimp_dir)
    {
      if (g_path_is_absolute (env_gimp_dir))
	gimp_dir = g_strdup (env_gimp_dir);
      else
	{
	  if (NULL != home_dir)
	    {
	      gimp_dir = g_strconcat (home_dir,
				      home_dir_sep,
				      env_gimp_dir,
				      NULL);
	    }
	  else
	    {
	      gimp_dir = g_strconcat (gimp_data_directory (),
				      G_DIR_SEPARATOR_S,
				      env_gimp_dir,
				      NULL);
	    }
	}
    }
  else
    {
#ifdef __EMX__       
	gimp_dir = g_strdup(__XOS2RedirRoot(GIMPDIR));
	return gimp_dir;  
#endif      
	if (NULL != home_dir)
	{
	  gimp_dir = g_strconcat (home_dir,
				  home_dir_sep,
				  GIMPDIR,
				  NULL);
	}
      else
	{
#ifndef G_OS_WIN32
	  g_message ("warning: no home directory.");
#endif
	  gimp_dir = g_strconcat (gimp_data_directory (),
				  G_DIR_SEPARATOR_S,
				  GIMPDIR,
				  ".",
				  g_get_user_name (),
				  NULL);
	}
    }

  return gimp_dir;
}

/**
 * gimp_data_directory:
 *
 * Returns the top directory for GIMP data. If the environment variable 
 * GIMP_DATADIR exists, that is used.  It should be an absolute pathname.
 * Otherwise, on Unix the compile-time defined directory is used.  On
 * Win32, the installation directory as deduced from the executable's
 * name is used.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 *
 * Returns: The top directory for GIMP data.
 **/
gchar*
gimp_data_directory (void)
{
  static gchar *gimp_data_dir = NULL;
  gchar *env_gimp_data_dir = NULL;
  
  if (gimp_data_dir != NULL)
    return gimp_data_dir;

  env_gimp_data_dir = g_getenv ("GIMP_DATADIR");

  if (NULL != env_gimp_data_dir)
    {
      if (!g_path_is_absolute (env_gimp_data_dir))
	g_error ("GIMP_DATADIR environment variable should be an absolute path.");
#ifndef __EMX__
      gimp_data_dir = g_strdup (env_gimp_data_dir);
#else      
      gimp_data_dir = g_strdup (__XOS2RedirRoot(env_gimp_data_dir));
#endif      
    }
  else
    {
#ifndef G_OS_WIN32
#ifndef __EMX__
      gimp_data_dir = DATADIR;
#else
      gimp_data_dir = g_strdup(__XOS2RedirRoot(DATADIR));
#endif
#else
      /* Figure it out from the executable name */
      gchar filename[MAX_PATH];
      gchar *sep1, *sep2;

      if (GetModuleFileName (NULL, filename, sizeof (filename)) == 0)
	g_error ("GetModuleFilename failed\n");
      
      /* If the executable file name is of the format
       * <foobar>\bin\gimp.exe of <foobar>\plug-ins\filter.exe, * use
       * <foobar>. Otherwise, use the directory where the executable
       * is.
       */

      sep1 = strrchr (filename, G_DIR_SEPARATOR);

      *sep1 = '\0';

      sep2 = strrchr (filename, G_DIR_SEPARATOR);

      if (sep2 != NULL)
	{
	  if (g_strcasecmp (sep2 + 1, "bin") == 0
	      || g_strcasecmp (sep2 + 1, "plug-ins") == 0)
	    *sep2 = '\0';
	}

      gimp_data_dir = g_strdup (filename);
#endif
    }
  return gimp_data_dir;
}
#endif



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

void	run (char *, int, GimpParam *, int *, GimpParam **);
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
stpui_get_thumbnail_data_function(void *image_ID_printc, gint *width, gint *height,
				  gint *bpp, gint page)
{
  return gimp_image_get_thumbnail_data((gint) image_ID_printc, width, height, bpp);
}

void
runit(char   *name,		/* I - Name of print program. */
     int    *nreturn_vals,	/* O - Number of return values */
     GimpParam **return_vals)	/* O - Return values */
{
  GimpDrawable	*drawable;	/* Drawable for image */
  GimpParam	*values;	/* Return values */
  gint32         drawable_ID;   /* drawable ID */
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;    /* return value of gimp_export_image() */
  gdouble xres, yres;
  const char *image_filename;
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

  image_filename = gimp_image_get_filename(image_ID_printc);
  if (strchr(image_filename, '/'))
    image_filename = strrchr(image_filename, '/') + 1;
  stpui_set_image_filename(image_filename);

  /*  eventually export the image */
  gimp_ui_init ("print", TRUE);
  export = gimp_export_image (&image_ID_printc, &drawable_ID, "Print",
				  (GIMP_EXPORT_CAN_HANDLE_RGB |
				   GIMP_EXPORT_CAN_HANDLE_GRAY |
				   GIMP_EXPORT_CAN_HANDLE_INDEXED |
				   GIMP_EXPORT_CAN_HANDLE_ALPHA));
  if( export == GIMP_EXPORT_CANCEL)
  {
     *nreturn_vals = 1;
     values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
     return;
  }

  /*
   * Get drawable...
   */

  drawable = gimp_drawable_get (drawable_ID);
  stpui_set_image_dimensions(drawable->width, drawable->height);
  gimp_image_get_resolution (image_ID_printc, &xres, &yres);
  stpui_set_image_resolution(xres, yres);
  image = Image_GimpDrawable_new(drawable, image_ID_printc);
  stp_set_float_parameter(gimp_vars.v, "AppGamma", gimp_gamma());

  /*
   * Get information from the dialog...
   */
   
  if(do_print_dialog (name))
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
   
         if (drawable->height > drawable->width)
	   gimp_tile_cache_ntiles ((drawable->height + gimp_tile_width () - 1) /
				   gimp_tile_width () + 1);
         else
	   gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
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
   
     /*
      * Detach from the drawable...
      */
     gimp_drawable_detach (drawable);
  }
  if( export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID_printc);
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

