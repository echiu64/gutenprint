/*
 * "$Id$"
 *
 *   EPSON ESC/P2 raster filter for the CUPS driver development kit.
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
 *
 * Contents:
 *
 *   Setup()        - Prepare the printer for printing.
 *   StartPage()    - Start a page of graphics.
 *   EndPage()      - Finish a page of graphics.
 *   Shutdown()     - Shutdown the printer.
 *   CompressData() - Compress a line of graphics.
 *   OutputLine()   - Output a line of graphics.
 *   main()         - Main entry and processing of driver.
 */

/*
 * Include necessary headers...
 */

#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "util.h"


/*
 * Model numbers...
 */

#define EPSON_9PIN	0
#define EPSON_24PIN	1
#define EPSON_COLOR	2
#define EPSON_PHOTO	3
#define EPSON_NEWCOLOR	4
#define EPSON_NEWPHOTO	5


/*
 * Macros...
 */

#define pwrite(s,n) fwrite((s), 1, (n), stdout)


/*
 * Globals...
 */

		/* All Printers */
unsigned char	*PixelBuffers[6],	/* Pixel buffers */
		*DotBuffers[64][6],	/* Output buffers */
		*Planes[6],		/* Output buffers */
		*CompBuffer;		/* Compression buffer */
int		NumPlanes,		/* Number of color planes */
		BitPlanes,		/* Number of bits per color */
		Feed;			/* Number of lines to skip */
void		*Dither;		/* Dither data */

		/* Stylus Printers */
int		DotBufferClear[64],	/* Non-zero if dot buffer is cleared */
		DotBufferSize,		/* Size of dot buffers */
		DotRow[64],		/* Current rows in buffer */
		DotRowMax,		/* Maximum row number in buffer */
		DotColStep,		/* Step for each output column */
		DotRowStep,		/* Step for each output line */
		DotRowFeed,		/* Amount to feed for interleave */
		DotRowIncr,		/* Increment for row buffer */
		DotRowCount,		/* Number of rows to output */
		DotRowStart[64],	/* Starting offsets for output */
		DotRowCurrent,		/* Current row */
		DotRowFirst,		/* First regular output line */
		DotSize;		/* Dot size (Pro 5000 only) */

		/* Dot-Matrix Printers */
int		DotBit,			/* Bit in buffers */
		DotBytes,		/* # bytes in a dot column */
		DotColumns,		/* # columns in 1/60 inch */
		LineCount,		/* # of lines processed */
		EvenOffset,		/* Offset into 'even' buffers */
		OddOffset,		/* Offset into 'odd' buffers */
		Shingling;		/* Shingle output? */

static double dot_sizes[] = { 0.550, 0.700, 1.0 };

static simple_dither_range_t variable_dither_ranges[] =
{
  { 0.137, 0x1, 0, 1 },
/*  { 0.233, 0x2, 0, 2 },
  { 0.333, 0x3, 0, 3 }, */
  { 0.550, 0x1, 1, 1 },
  { 0.700, 0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static simple_dither_range_t standard_dither_ranges[] =
{
  { 0.550, 0x1, 1, 1 },
  { 0.7,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static simple_dither_range_t mis_sixtone_ranges[] =
{
  { 0.15, 0x000001, 1, 1 },	/* LC */
  { 0.25, 0x000010, 1, 1 },	/* C */
  { 0.45, 0x000100, 1, 1 },	/* LM */
  { 0.50, 0x001000, 1, 1 },	/* Y */
  { 0.75, 0x010000, 1, 1 },	/* M */
  { 1.00, 0x100000, 1, 1 }	/* K */
};



/*
 * Prototypes...
 */

void	Setup(void);
void	StartPage(const ppd_file_t *, const cups_page_header_t *);
void	EndPage(const ppd_file_t *, const cups_page_header_t *);
void	Shutdown(void);

void	CompressData(const ppd_file_t *, const unsigned char *, const int,
	             const int, int, const int, const int, const int,
		     const int);
void	OutputBand(const ppd_file_t *, const cups_page_header_t *, const int,
	           const int);
void	OutputLine(const ppd_file_t *, const cups_page_header_t *, const int);
void	OutputRows(const ppd_file_t *, const cups_page_header_t *, const int);
void	ProcessLine(const ppd_file_t *, cups_raster_t *,
	            const cups_page_header_t *, const int);


/*
 * 'Setup()' - Prepare the printer for printing.
 */

void
Setup(void)
{
 /*
  * Send a reset sequence.
  */

  printf("\033@");
}


/*
 * 'StartPage()' - Start a page of graphics.
 */

void
StartPage(const ppd_file_t         *ppd,	/* I - PPD file */
          const cups_page_header_t *header)	/* I - Page header */
{
  int		n, t;				/* Numbers */
  int		plane;				/* Looping var */
  unsigned char	*ptr;				/* Current pixel buffer */
  int		y,				/* Page position */
		subrow,				/* Current subrow */
		modrow;				/* Softweave divisor */
  float		density;			/* Density value */


 /*
  * Debugging info...
  */

  fprintf(stderr, "DEBUG: StartPage...\n");
  fprintf(stderr, "DEBUG: MediaClass = \"%s\"\n", header->MediaClass);
  fprintf(stderr, "DEBUG: MediaColor = \"%s\"\n", header->MediaColor);
  fprintf(stderr, "DEBUG: MediaType = \"%s\"\n", header->MediaType);
  fprintf(stderr, "DEBUG: OutputType = \"%s\"\n", header->OutputType);

  fprintf(stderr, "DEBUG: AdvanceDistance = %d\n", header->AdvanceDistance);
  fprintf(stderr, "DEBUG: AdvanceMedia = %d\n", header->AdvanceMedia);
  fprintf(stderr, "DEBUG: Collate = %d\n", header->Collate);
  fprintf(stderr, "DEBUG: CutMedia = %d\n", header->CutMedia);
  fprintf(stderr, "DEBUG: Duplex = %d\n", header->Duplex);
  fprintf(stderr, "DEBUG: HWResolution = [ %d %d ]\n", header->HWResolution[0],
          header->HWResolution[1]);
  fprintf(stderr, "DEBUG: ImagingBoundingBox = [ %d %d %d %d ]\n",
          header->ImagingBoundingBox[0], header->ImagingBoundingBox[1],
          header->ImagingBoundingBox[2], header->ImagingBoundingBox[3]);
  fprintf(stderr, "DEBUG: InsertSheet = %d\n", header->InsertSheet);
  fprintf(stderr, "DEBUG: Jog = %d\n", header->Jog);
  fprintf(stderr, "DEBUG: LeadingEdge = %d\n", header->LeadingEdge);
  fprintf(stderr, "DEBUG: Margins = [ %d %d ]\n", header->Margins[0],
          header->Margins[1]);
  fprintf(stderr, "DEBUG: ManualFeed = %d\n", header->ManualFeed);
  fprintf(stderr, "DEBUG: MediaPosition = %d\n", header->MediaPosition);
  fprintf(stderr, "DEBUG: MediaWeight = %d\n", header->MediaWeight);
  fprintf(stderr, "DEBUG: MirrorPrint = %d\n", header->MirrorPrint);
  fprintf(stderr, "DEBUG: NegativePrint = %d\n", header->NegativePrint);
  fprintf(stderr, "DEBUG: NumCopies = %d\n", header->NumCopies);
  fprintf(stderr, "DEBUG: Orientation = %d\n", header->Orientation);
  fprintf(stderr, "DEBUG: OutputFaceUp = %d\n", header->OutputFaceUp);
  fprintf(stderr, "DEBUG: PageSize = [ %d %d ]\n", header->PageSize[0],
          header->PageSize[1]);
  fprintf(stderr, "DEBUG: Separations = %d\n", header->Separations);
  fprintf(stderr, "DEBUG: TraySwitch = %d\n", header->TraySwitch);
  fprintf(stderr, "DEBUG: Tumble = %d\n", header->Tumble);
  fprintf(stderr, "DEBUG: cupsWidth = %d\n", header->cupsWidth);
  fprintf(stderr, "DEBUG: cupsHeight = %d\n", header->cupsHeight);
  fprintf(stderr, "DEBUG: cupsMediaType = %d\n", header->cupsMediaType);
  fprintf(stderr, "DEBUG: cupsBitsPerColor = %d\n", header->cupsBitsPerColor);
  fprintf(stderr, "DEBUG: cupsBitsPerPixel = %d\n", header->cupsBitsPerPixel);
  fprintf(stderr, "DEBUG: cupsBytesPerLine = %d\n", header->cupsBytesPerLine);
  fprintf(stderr, "DEBUG: cupsColorOrder = %d\n", header->cupsColorOrder);
  fprintf(stderr, "DEBUG: cupsColorSpace = %d\n", header->cupsColorSpace);
  fprintf(stderr, "DEBUG: cupsCompression = %d\n", header->cupsCompression);
  fprintf(stderr, "DEBUG: cupsRowCount = %d\n", header->cupsRowCount);
  fprintf(stderr, "DEBUG: cupsRowFeed = %d\n", header->cupsRowFeed);
  fprintf(stderr, "DEBUG: cupsRowStep = %d\n", header->cupsRowStep);

 /*
  * Send a reset sequence.
  */

  printf("\033@");

 /*
  * See which type of printer we are using...
  */

  switch (ppd->model_number)
  {
    case EPSON_9PIN :
    case EPSON_24PIN :
        printf("\033P");		/* Set 10 CPI */

	if (header->HWResolution[0] == 360 || header->HWResolution[0] == 240)
	{
	  printf("\033x1");		/* LQ printing */
	  printf("\033U1");		/* Unidirectional */
	}
	else
	{
	  printf("\033x0");		/* Draft printing */
	  printf("\033U0");		/* Bidirectional */
	}

	printf("\033l%c\033Q%c", 0,	/* Side margins */
                      (int)(10.0 * header->PageSize[0] / 72.0 + 0.5));
	printf("\033C%c%c", 0,		/* Page length */
                      (int)(header->PageSize[1] / 72.0 + 0.5));
	printf("\033N%c", 0);		/* Bottom margin */

       /*
	* Setup various buffer limits...
	*/

        DotBytes      = header->cupsRowCount / 8;
	DotBufferSize = DotBytes * header->cupsWidth;
	DotColumns    = header->HWResolution[0] / 60;
        Shingling     = 0;

        if (ppd->model_number == EPSON_9PIN)
	  printf("\033\063\030");	/* Set line feed */
	else
	  switch (header->HWResolution[0])
	  {
	    case 60:
	    case 120 :
        	printf("\033\063\030");	/* Set line feed */
		break;

	    case 180 :
	    case 360 :
        	Shingling = 1;

        	if (header->HWResolution[1] == 180)
        	  printf("\033\063\010");/* Set line feed */
		else
        	  printf("\033+\010");	/* Set line feed */
        	break;
	  }
        break;

    case EPSON_COLOR :
    case EPSON_PHOTO :
    case EPSON_NEWCOLOR :
    case EPSON_NEWPHOTO :
       /*
        * Set bits per pixel from compression field...
	*/

	BitPlanes = header->cupsCompression;

       /*
        * Set microweave/softweave variables...
	*/

	if (header->cupsRowCount <= 1)
	{
	  DotRowMax  = 1;
	  DotColStep = 1;
	  DotRowStep = 1;
	  DotRowFeed = 1;
	}
	else
	{
	  DotRowMax  = header->cupsRowCount;
	  DotRowFeed = header->cupsRowFeed;
	  DotRowStep = header->cupsRowStep % 100;
	  DotColStep = header->cupsRowStep / 100;
	  if (DotColStep == 0)
	    DotColStep ++;
	}

       /*
	* Compute remaining softweave settings...
	*/

	DotRowCurrent = 0;
	DotRowCount   = DotRowMax / DotRowStep;

	DotBufferSize = (header->cupsWidth / DotColStep * BitPlanes + 7) / 8 *
                	DotRowCount;
	DotRowIncr    = DotRowFeed * DotRowStep * DotColStep;

	fprintf(stderr, "DEBUG: DotColStep = %d\n", DotColStep);
	fprintf(stderr, "DEBUG: DotRowMax = %d\n", DotRowMax);
	fprintf(stderr, "DEBUG: DotRowStep = %d\n", DotRowStep);
	fprintf(stderr, "DEBUG: DotRowFeed = %d\n", DotRowFeed);
	fprintf(stderr, "DEBUG: DotRowCount = %d\n", DotRowCount);
	fprintf(stderr, "DEBUG: DotRowIncr = %d\n", DotRowIncr);

       /*
	* Set graphics mode...
	*/

	pwrite("\033(G\001\000\001", 6);	/* Graphics mode */

       /*
	* Set the line feed increment...
	*/

	if (ppd->model_number < EPSON_NEWCOLOR)
	{
	  pwrite("\033(U\001\000", 5);
	  putchar(3600 / header->HWResolution[1]);
	}
	else
	{
	  pwrite("\033(U\005\000", 5);
	  putchar(1440 / header->HWResolution[1]);
	  putchar(DotRowStep * 1440 / header->HWResolution[1]);
	  putchar(DotColStep * 1440 / header->HWResolution[0]);
	  putchar(0xa0);	/* n/1440ths... */
	  putchar(0x05);
	}

       /*
	* Set the media size...
	*/

	n = header->PageSize[1] * header->HWResolution[1] / 72.0;

	pwrite("\033(C\002\000", 5);		/* Page length */
	putchar(n);
	putchar(n >> 8);

	t = (ppd->sizes[1].length - ppd->sizes[1].top) *
	    header->HWResolution[1] / 72.0;

	pwrite("\033(c\004\000", 5);		/* Top & bottom margins */
	putchar(t);
	putchar(t >> 8);
	putchar(n);
	putchar(n >> 8);

	if (ppd->model_number >= EPSON_NEWCOLOR)
	{
	 /*
	  * Set page size (expands bottom margin)...
	  */

	  pwrite("\033(S\010\000", 5);

	  n = header->PageSize[0] * header->HWResolution[0] / 72;
	  putchar(n);
	  putchar(n >> 8);
	  putchar(n >> 16);
	  putchar(n >> 24);

	  n = header->PageSize[1] * header->HWResolution[1] / 72;
	  putchar(n);
	  putchar(n >> 8);
	  putchar(n >> 16);
	  putchar(n >> 24);
	}

	if (header->HWResolution[1] == 720 && DotRowStep == 1)
	  pwrite("\033(i\001\000\001", 6);	/* Microweave on */
	else
	  pwrite("\033(i\001\000\000", 6);	/* Microweave off */

        if (BitPlanes > 1)
	  pwrite("\033(e\002\000\000\020", 7);	/* Variable dots */
	else
          switch (header->HWResolution[0])
	  {
	    case 360 :
    		pwrite("\033(e\002\000\000\003", 7);	/* Double dots */
		break;
	    case 720 :
	        if (header->cupsMediaType > 1)
		{
    		  pwrite("\033(e\002\000\000\001", 7);	/* Small dots */
		  break;
		}
	    case 1440 :
    		pwrite("\033(e\002\000\000\004", 7);	/* Micro dots */
		break;
	  }

	pwrite("\033(V\002\000\000\000", 7);	/* Set absolute position 0 */

        DotBytes   = 0;
	DotColumns = 0;
        Shingling  = 0;
        break;
  }

 /*
  * Set other stuff...
  */

  if (header->cupsColorSpace == CUPS_CSPACE_CMY)
    NumPlanes = 3;
  else if (header->cupsColorSpace == CUPS_CSPACE_KCMY)
    NumPlanes = 4;
  else if (header->cupsColorSpace == CUPS_CSPACE_KCMYcm)
    NumPlanes = 6;
  else
    NumPlanes = 1;

  Feed = 0;				/* No blank lines yet */

 /*
  * Allocate memory for a line/row of graphics...
  */

  if (header->cupsBitsPerColor > 1)
  {
   /*
    * Allocate buffers for the incoming pixel data...
    */

    PixelBuffers[0] = malloc(header->cupsWidth * NumPlanes);

    for (plane = 1; plane < NumPlanes; plane ++)
      PixelBuffers[plane] = PixelBuffers[0] + plane * header->cupsWidth;

   /*
    * Compute the output density...
    */

    density = 720.0 * 720.0 / header->HWResolution[0] / header->HWResolution[1];
    density /= DotColStep;
    if (BitPlanes == 2)
      density *= 3.3;
    if (BitPlanes == 2 && header->HWResolution[0] == 720)
      density /= 1.5;

    fprintf(stderr, "DEBUG: raw density = %.4f\n", density);

    if (density > 1.0)
      density = 1.0;

    if (header->cupsMediaType == 1)
      density *= 0.5;

    fprintf(stderr, "DEBUG: capped density = %.4f\n", density);

    Dither = init_dither(header->cupsWidth, header->cupsWidth, 1, 1,
                         header->OutputType);
    if (header->HWResolution[0] > header->HWResolution[1])
      Dither = init_dither(header->cupsWidth, header->cupsWidth, 1,
                           header->HWResolution[0] / header->HWResolution[1],
                           BitPlanes > 1 ? "Adaptive Hybrid" : "Ordered");
    else
      Dither = init_dither(header->cupsWidth, header->cupsWidth,
                           header->HWResolution[1] / header->HWResolution[0], 1,
                           BitPlanes > 1 ? "Adaptive Hybrid" : "Ordered");

    dither_set_black_levels(Dither, 1.0, 1.0, 1.0);
    if (NumPlanes > 4)
      dither_set_black_lower(Dither, .4 / BitPlanes + .1);
    else
      dither_set_black_lower(Dither, .25 / BitPlanes);

    if (header->cupsMediaType == 3)
      dither_set_black_upper(Dither, .999);
    else
      dither_set_black_upper(Dither, .6);

    if (BitPlanes == 2)
    {
      if (NumPlanes > 4)
	dither_set_adaptive_divisor(Dither, 8);
      else
	dither_set_adaptive_divisor(Dither, 16);
    }  
    else
      dither_set_adaptive_divisor(Dither, 4);

    if (BitPlanes == 2)
    {
      int dsize = (sizeof(variable_dither_ranges) /
		   sizeof(simple_dither_range_t));


      dither_set_y_ranges_simple(Dither, 3, dot_sizes, density);
      dither_set_k_ranges_simple(Dither, 3, dot_sizes, density);
      if (NumPlanes > 4)
      {
	dither_set_transition(Dither, .7);
	dither_set_c_ranges(Dither, dsize, variable_dither_ranges,
			    density);
	dither_set_m_ranges(Dither, dsize, variable_dither_ranges,
			    density);
      }
      else
      {
	dither_set_transition(Dither, .5);
	dither_set_c_ranges_simple(Dither, 3, dot_sizes, density);
	dither_set_m_ranges_simple(Dither, 3, dot_sizes, density);
      }
    }
    else if (NumPlanes > 4)
      dither_set_light_inks(Dither, .33, .33, 0.0, density * .75);

    if (BitPlanes > 1)
      dither_set_ink_spread(Dither, 14);
    else
      dither_set_ink_spread(Dither, 13);

    dither_set_density(Dither, density);
  }
  else
    Dither = NULL;

  Planes[0] = malloc(header->cupsBytesPerLine);
  for (plane = 1; plane < NumPlanes; plane ++)
    Planes[plane] = Planes[0] + plane * header->cupsBytesPerLine / NumPlanes;

  if (header->cupsCompression || DotBytes)
    CompBuffer = calloc(2, DotBufferSize);

  if (DotBytes)
  {
   /*
    * Allocate memory for dot-matrix buffers...
    */

    DotBuffers[0][0] = calloc(DotBytes, header->cupsWidth * (Shingling + 1));
    DotBuffers[0][1] = DotBuffers[0][0] + DotBytes * header->cupsWidth;
    DotBit           = 128;
    LineCount        = 0;
    EvenOffset       = 0;
    OddOffset        = 0;
  }
  else
  {
   /*
    * Allocate memory for inkjet buffers...
    */

    ptr = calloc(DotBufferSize * DotColStep * DotRowStep * NumPlanes, 1);

    for (subrow = 0; subrow < (DotColStep * DotRowStep); subrow ++)
      for (plane = 0; plane < NumPlanes; plane ++, ptr += DotBufferSize)
      {
	DotBuffers[subrow][plane] = ptr;
	DotBufferClear[subrow]    = 1;

	fprintf(stderr, "DEBUG: Allocated %d bytes for subrow %d, plane %d (ptr = %p)\n",
        	DotBufferSize, subrow, plane, DotBuffers[subrow][plane]);
      }

    if (DotRowMax > 1)
    {
     /*
      * Compute the starting row values.
      */

      modrow      = DotColStep * DotRowStep;
      DotRowFirst = modrow - 1;

      for (n = DotColStep * DotRowStep, subrow = DotRowFirst, y = DotRowFirst;
	   n > 0;
	   n --, y += DotRowFeed)
      {
	DotRow[subrow]      = subrow;
	DotRowStart[subrow] = y / DotRowStep;
	fprintf(stderr, "DEBUG: DotRowStart[%d] = %d\n", subrow,
		DotRowStart[subrow]);

	subrow = (subrow + DotRowFeed) % modrow;
      }
    }
  }
}


/*
 * 'EndPage()' - Finish a page of graphics.
 */

void
EndPage(const ppd_file_t         *ppd,	/* I - PPD file info */
        const cups_page_header_t *header)/* I - Page header */
{
  int	bands;				/* Number of bands to write */
  int	y;				/* Current scanline */
  int	pass;				/* Current pass */
  int	subrow;				/* Current subrow */


  if (DotBytes)
  {
   /*
    * Flush remaining graphics as needed...
    */

    if (!Shingling)
      OutputRows(ppd, header, 0);
    else if (OddOffset > EvenOffset)
    {
      OutputRows(ppd, header, 1);
      OutputRows(ppd, header, 0);
    }
    else
    {
      OutputRows(ppd, header, 0);
      OutputRows(ppd, header, 1);
    }
  }
  else if (DotRowMax > 1)
  {
   /*
    * Figure out how many bands are left to write...
    */

    bands = DotRowStep * DotColStep;

   /*
    * Loop until all bands are written...
    */

    for (y = header->cupsHeight; bands > 0; y ++)
    {
      subrow = y % DotRowStep;

      for (pass = DotColStep; pass > 0 && bands > 0; pass --)
      {
	if ((y - DotRow[subrow]) >= DotRowMax)
	{
	  OutputBand(ppd, header, subrow, y);
	  bands --;
	}

	subrow += DotRowStep;
      }
    }
  }

 /*
  * Eject the current page...
  */

  putchar(12);		/* Form feed */

 /*
  * Free memory...
  */

  free(Planes[0]);

  if (header->cupsBitsPerColor > 1)
  {
    free(PixelBuffers[0]);
    free_dither(Dither);
  }

  free(DotBuffers[0][0]);

  if (header->cupsCompression || DotBytes)
    free(CompBuffer);
}


/*
 * 'Shutdown()' - Shutdown the printer.
 */

void
Shutdown(void)
{
 /*
  * Send a reset sequence.
  */

  printf("\033@");
}


/*
 * 'CompressData()' - Compress a line of graphics.
 */

void
CompressData(const ppd_file_t    *ppd,	/* I - PPD file information */
             const unsigned char *line,	/* I - Data to compress */
             const int           length,/* I - Number of bytes */
	     const int           plane,	/* I - Color plane */
	     int                 type,	/* I - Type of compression */
	     const int           rows,	/* I - Number of lines to write */
	     const int           xstep,	/* I - Spacing between columns */
	     const int           ystep,	/* I - Spacing between lines */
	     const int           offset)/* I - Head offset */
{
  register const unsigned char *line_ptr,/* Current byte pointer */
        	*line_end,		/* End-of-line byte pointer */
        	*start;			/* Start of compression sequence */
  register unsigned char *comp_ptr;	/* Pointer into compression buffer */
  register int  count;			/* Count of bytes for output */
  register int	bytes;			/* Number of bytes per row */
  static int	ctable[6] = { 0, 2, 1, 4, 18, 17 };
					/* KCMYcm color values */


  switch (type)
  {
    case 0 :
       /*
	* Do no compression...
	*/

	line_ptr = (const unsigned char *)line;
	line_end = (const unsigned char *)line + length;
	break;

    default :
       /*
        * Do TIFF pack-bits encoding...
        */

	line_ptr = (const unsigned char *)line;
	line_end = (const unsigned char *)line + length;
	comp_ptr = CompBuffer;

	while (line_ptr < line_end && (comp_ptr - CompBuffer) < length)
	{
	  if ((line_ptr + 1) >= line_end)
	  {
	   /*
	    * Single byte on the end...
	    */

	    *comp_ptr++ = 0x00;
	    *comp_ptr++ = *line_ptr++;
	  }
	  else if (line_ptr[0] == line_ptr[1])
	  {
	   /*
	    * Repeated sequence...
	    */

	    line_ptr ++;
	    count = 2;

	    while (line_ptr < (line_end - 1) &&
        	   line_ptr[0] == line_ptr[1] &&
        	   count < 127)
	    {
              line_ptr ++;
              count ++;
	    }

	    *comp_ptr++ = 257 - count;
	    *comp_ptr++ = *line_ptr++;
	  }
	  else
	  {
	   /*
	    * Non-repeated sequence...
	    */

	    start    = line_ptr;
	    line_ptr ++;
	    count    = 1;

	    while (line_ptr < (line_end - 1) &&
        	   line_ptr[0] != line_ptr[1] &&
        	   count < 127)
	    {
              line_ptr ++;
              count ++;
	    }

	    *comp_ptr++ = count - 1;

	    memcpy(comp_ptr, start, count);
	    comp_ptr += count;
	  }
	}

        if ((comp_ptr - CompBuffer) < length)
	{
          line_ptr = (const unsigned char *)CompBuffer;
          line_end = (const unsigned char *)comp_ptr;
	}
	else
	{
	  type     = 0;
	  line_ptr = (const unsigned char *)line;
	  line_end = (const unsigned char *)line + length;
	}
	break;
  }

 /*
  * Position the print head...
  */

  putchar(0x0d);

  if (offset)
  {
    if (BitPlanes == 1)
      pwrite("\033(\\\004\000\240\005", 7);
    else
      printf("\033\\");

    putchar(offset);
    putchar(offset >> 8);
  }

 /*
  * Send the graphics...
  */

  if (ppd->model_number >= EPSON_NEWCOLOR)
  {
    bytes = length / rows;

    printf("\033i");
    putchar(ctable[plane]);
    putchar(type != 0);
    putchar(BitPlanes);
    putchar(bytes & 255);
    putchar(bytes >> 8);
    putchar(rows & 255);
    putchar(rows >> 8);
  }
  else
  {
   /*
    * Set the color if necessary...
    */

    if (NumPlanes > 1)
    {
      if (plane > 3)
	printf("\033(r%c%c%c%c", 2, 0, 1, ctable[plane] & 15);
      else
	printf("\033r%c", ctable[plane]);
    }

    bytes = length / rows * 8;

    printf("\033.");
    putchar(type != 0);
    putchar(ystep);
    putchar(xstep);
    putchar(rows);
    putchar(bytes & 255);
    putchar(bytes >> 8);
  }

  pwrite(line_ptr, line_end - line_ptr);
}


/*
 * 'OutputBand()' - Output a band of graphics.
 */

void
OutputBand(const ppd_file_t         *ppd,	/* I - PPD file */
           const cups_page_header_t *header,	/* I - Page header */
	   const int                subrow,	/* I - Subrow to output */
           const int                y)		/* I - Current row */
{
  int	plane,				/* Current color plane */
	length,				/* Length of band in bytes */
	rows;				/* Number of lines to write out */
  int	xstep,				/* Spacing between columns */
	ystep;				/* Spacing between rows */


 /*
  * Interleaved ESC/P2 graphics...
  */

  Feed    += DotRow[subrow] - DotRowCurrent;
  DotRowCurrent = DotRow[subrow];

#ifdef DEBUG
  fprintf(stderr, "DEBUG: Printing subrow %d, DotRow[%d] = %d, "
                  "y = %d, Feed = %d\n",
          subrow, subrow, DotRow[subrow], y, Feed);
#endif /* DEBUG */

 /*
  * If DotRow[subrow] < DotRowFirst, then this is the first bunch of rows
  * and we need to restrict things accordingly...
  */

  if (DotRow[subrow] < DotRowFirst)
    rows = DotRowStart[subrow];
  else
    rows = DotRowCount;

  while ((DotRow[subrow] + rows * DotRowStep) >= header->cupsHeight && rows > 0)
    rows --;
    
 /*
  * If there are any rows left to be printed, do so...
  */

  if (rows > 0 && !DotBufferClear[subrow])
  {
   /*
    * Compute step/length values...
    */

    length = DotBufferSize / DotRowCount;
    xstep  = 3600 * DotColStep / header->HWResolution[0];
    ystep  = 3600 * DotRowStep / header->HWResolution[1];

   /*
    * Loop through each color plane, printing as needed...
    */

    for (plane = 0; plane < NumPlanes; plane ++)
    {
      if (!CheckBytes(DotBuffers[subrow][plane], rows * length))
      {
	if (Feed > 0)
	{
	  pwrite("\033(v\002\000", 5);
	  putchar(Feed & 255);
	  putchar(Feed >> 8);

	  Feed = 0;
	}

	CompressData(ppd, DotBuffers[subrow][plane], rows * length, plane,
	             header->cupsCompression, rows, xstep, ystep,
                     subrow / DotRowStep);
      }

      if (DotRow[subrow] >= DotRowFirst)
	memset(DotBuffers[subrow][plane], 0, DotBufferSize);
    }

   /*
    * If this is one of the initial rows, shift the buffers...
    */

    if (DotRow[subrow] < DotRowFirst)
    {
      for (plane = 0; plane < NumPlanes; plane ++)
      {
       /*
        * Didn't send entire buffer, so copy what's left...
	*/

	memcpy(DotBuffers[subrow][plane],
	       DotBuffers[subrow][plane] + rows * length,
	       (DotRowCount - rows) * length);
	memset(DotBuffers[subrow][plane] + (DotRowCount - rows) * length,
	       0, rows * length);
      }
    }
    else
      DotBufferClear[subrow] = 1;
  }

 /*
  * Update the row value as needed...
  */

  DotRow[subrow] += rows * DotRowStep;

 /*
  * See if we didn't output anything this time around; if so, flush the
  * output buffers.
  */

  if (Feed)
    fflush(stdout);
}


/*
 * 'OutputLine()' - Output a line of graphics.
 */

void
OutputLine(const ppd_file_t         *ppd,	/* I - PPD file */
           const cups_page_header_t *header,	/* I - Page header */
	   const int                y)		/* I - Current line */
{
  if (ppd->model_number <= EPSON_24PIN)
  {
    int			width;
    unsigned char	*tempptr,
			*evenptr,
			*oddptr;
    register int	x;
    unsigned char	bit;
    const unsigned char	*pixel;
    unsigned char 	*temp;


   /*
    * Collect bitmap data in the line buffers and write after each buffer.
    */

    for (x = header->cupsWidth, bit = 128, pixel = Planes[0],
             temp = CompBuffer;
	 x > 0;
	 x --, temp ++)
    {
      if (*pixel & bit)
        *temp |= DotBit;

      if (bit > 1)
	bit >>= 1;
      else
      {
	bit = 128;
	pixel ++;
      }
    }

    if (DotBit > 1)
      DotBit >>= 1;
    else
    {
     /*
      * Copy the holding buffer to the output buffer, shingling as necessary...
      */

      if (Shingling && LineCount != 0)
      {
       /*
        * Shingle the output...
        */

        if (LineCount & 1)
        {
          evenptr = DotBuffers[0][1] + OddOffset;
          oddptr  = DotBuffers[0][0] + EvenOffset + DotBytes;
        }
        else
        {
          evenptr = DotBuffers[0][0] + EvenOffset;
          oddptr  = DotBuffers[0][1] + OddOffset + DotBytes;
        }

        for (width = header->cupsWidth, tempptr = CompBuffer;
             width > 0;
             width -= 2, tempptr += 2, oddptr += DotBytes * 2,
	         evenptr += DotBytes * 2)
        {
          evenptr[0] = tempptr[0];
          oddptr[0]  = tempptr[1];
        }
      }
      else
      {
       /*
        * Don't shingle the output...
        */

        for (width = header->cupsWidth, tempptr = CompBuffer,
                 evenptr = DotBuffers[0][0] + EvenOffset;
             width >= 0;
             width --, tempptr ++, evenptr += DotBytes)
          *evenptr = tempptr[0];
      }

      if (Shingling && LineCount != 0)
      {
	EvenOffset ++;
	OddOffset ++;

	if (EvenOffset == DotBytes)
	{
	  EvenOffset = 0;
	  OutputRows(ppd, header, 0);
	}

	if (OddOffset == DotBytes)
	{
          OddOffset = 0;
	  OutputRows(ppd, header, 1);
	}
      }
      else
      {
	EvenOffset ++;

	if (EvenOffset == DotBytes)
	{
          EvenOffset = 0;
	  OutputRows(ppd, header, 0);
	}
      }

      DotBit = 128;
      LineCount ++;

      memset(CompBuffer, 0, header->cupsWidth);
    }
  }
  else
  {
    int	plane;		/* Current plane */
    int	bytes;		/* Bytes per plane */
    int	xstep, ystep;	/* X & Y resolutions */


   /*
    * Write a single line of bitmap data as needed...
    */

    xstep = 3600 / header->HWResolution[0];
    ystep = 3600 / header->HWResolution[1];
    bytes = header->cupsBytesPerLine / NumPlanes;

    for (plane = 0; plane < NumPlanes; plane ++)
    {
      if (CheckBytes(DotBuffers[0][plane], DotBufferSize))
	continue;

      if (Feed > 0)
      {
	pwrite("\033(v\002\000", 5);
	putchar(Feed & 255);
	putchar(Feed >> 8);
	Feed = 0;
      }

      CompressData(ppd, DotBuffers[0][plane], DotBufferSize, plane,
                   header->cupsCompression, 1, xstep, ystep, 0);
    }

    Feed ++;
  }
}


/*
 * 'OutputRows()' - Output 8, 24, or 48 rows.
 */

void
OutputRows(const ppd_file_t         *ppd,	/* I - PPD file */
           const cups_page_header_t *header,	/* I - Page image header */
           int                      row)	/* I - Row number (0 or 1) */
{
  unsigned	i, n;				/* Looping vars */
  int		dot_count,			/* Number of bytes to print */
                dot_min;			/* Minimum number of bytes */
  unsigned char *dot_ptr;			/* Pointer to print data */


  dot_min = DotBytes * DotColumns;

  if (DotBuffers[0][row][0] != 0 ||
      memcmp(DotBuffers[0][row], DotBuffers[0][row] + 1,
             header->cupsWidth * DotBytes - 1))
  {
   /*
    * Skip leading space...
    */

    i         = 0;
    dot_count = header->cupsWidth * DotBytes;
    dot_ptr   = DotBuffers[0][row];

    while (dot_count >= dot_min && dot_ptr[0] == 0 &&
           memcmp(dot_ptr, dot_ptr + 1, dot_min - 1) == 0)
    {
      i         ++;
      dot_ptr   += dot_min;
      dot_count -= dot_min;
    }

   /*
    * Skip trailing space...
    */

    while (dot_count >= dot_min && dot_ptr[dot_count - dot_min] == 0 &&
           memcmp(dot_ptr + dot_count - dot_min,
	          dot_ptr + dot_count - dot_min + 1, dot_min - 1) == 0)
      dot_count -= dot_min;

   /*
    * Position print head for printing...
    */

    putchar(0x1b);
    putchar('$');
    putchar(i & 255);
    putchar(i >> 8);

   /*
    * Start bitmap graphics for this line...
    */

    printf("\033*");			/* Select bit image */
    switch (header->HWResolution[0])
    {
      case 60 : /* 60x60/72 DPI gfx */
          putchar(0);
          break;
      case 120 : /* 120x60/72 DPI gfx */
          putchar(1);
          break;
      case 180 : /* 180 DPI gfx */
          putchar(39);
          break;
      case 240 : /* 240x72 DPI gfx */
          putchar(3);
          break;
      case 360 : /* 360x180/360 DPI gfx */
	  if (header->HWResolution[1] == 180)
	  {
            if (Shingling && LineCount != 0)
              putchar(40);		/* 360x180 fast */
            else
              putchar(41);		/* 360x180 slow */
	  }
	  else
          {
	    if (Shingling && LineCount != 0)
              putchar(72);		/* 360x360 fast */
            else
              putchar(73);		/* 360x360 slow */
          }
          break;
    }

    n = (unsigned)dot_count / DotBytes;
    putchar(n & 255);
    putchar(n / 256);

   /*
    * Write the graphics data...
    */

    pwrite(dot_ptr, dot_count);
  }

 /*
  * Feed the paper...
  */

  putchar('\n');

  if (Shingling && row == 1)
  {
    if (header->HWResolution[1] == 360)
      printf("\n\n\n\n");
    else
      printf("\n");
  }

 /*
  * Clear the buffer...
  */

  memset(DotBuffers[0][row], 0, header->cupsWidth * DotBytes);
}


/*
 * 'ProcessLine()' - Read graphics from the page stream and output as needed.
 */

void
ProcessLine(const ppd_file_t         *ppd,	/* I - PPD file */
            cups_raster_t            *ras,	/* I - Raster stream */
            const cups_page_header_t *header,	/* I - Page header */
            const int                y)		/* I - Current scanline */
{
  int		plane,		/* Current color plane */
		subrow,		/* Subrow for interleaved output */
		offset,		/* Offset to current line */
		pass,		/* Pass number */
		width,		/* Width of graphics */
		length;		/* Length of line */
  unsigned char	*ptr,		/* Pointer into buffer */
		*end,		/* End of buffer */
		temp;		/* Temporary byte */


 /*
  * Compute the subrow, offset, and length of the image data...
  */

  if (DotRowMax == 1)
  {
   /*
    * Standard ESC/P2 graphics...
    */

    width  = header->cupsWidth;
    length = (width + 7) / 8;
    subrow = 0;
    offset = 0;
  }
  else
  {
   /*
    * Softweaved ESC/P2 graphics...
    */

    width  = header->cupsWidth / DotColStep;
    length = DotBufferSize / DotRowCount;
    subrow = y % DotRowStep;

    if ((y - DotRow[subrow]) >= DotRowMax)
      OutputBand(ppd, header, subrow, y);

    offset = ((y - DotRow[subrow]) / DotRowStep) * length;
  }

 /*
  * Read the image data as needed...
  */

  if (header->cupsBitsPerColor == 1)
  {
   /*
    * Pre-dithered data...
    */

    for (plane = 0; plane < NumPlanes; plane ++)
    {
      cupsRasterReadPixels(ras, DotBuffers[subrow][plane] + offset, length);

     /*
      * Do ink depletion for 720/1440 DPI printing...
      */

      if (header->HWResolution[0] >= 720)
      {
	for (ptr = DotBuffers[subrow][plane] + offset, end = ptr + length;
	     ptr < end;)
	{
	 /*
	  * Grab the current byte...
	  */

	  temp = *ptr;

	 /*
	  * Check adjacent bits...
	  */

	  if ((temp & 0xc0) == 0xc0)
            temp &= 0xbf;
	  if ((temp & 0x60) == 0x60)
            temp &= 0xdf;
	  if ((temp & 0x30) == 0x30)
            temp &= 0xef;
	  if ((temp & 0x18) == 0x18)
            temp &= 0xf7;
	  if ((temp & 0x0c) == 0x0c)
            temp &= 0xfb;
	  if ((temp & 0x06) == 0x06)
            temp &= 0xfd;
	  if ((temp & 0x03) == 0x03)
            temp &= 0xfe;

	  *ptr++ = temp;

	 /*
	  * Check the last bit in the current byte and the first bit in the
	  * next byte...
	  */

	  if ((temp & 0x01) && ptr < end && *ptr & 0x80)
            *ptr &= 0x7f;
	}
      }
    }

    if (DotRowMax == 1)
      OutputLine(ppd, header, y);
    else
      DotBufferClear[subrow] = 0;
  }
  else
  {
   /*
    * Read 8-bit per color data...
    */

    cupsRasterReadPixels(ras, PixelBuffers[0], header->cupsBytesPerLine);

   /*
    * See if we even need to dither it...
    */

    if (CheckBytes(PixelBuffers[0], header->cupsBytesPerLine))
    {
     /*
      * Nope; update the feed value as needed...
      */

      if (DotRowMax == 1)
      {
        Feed ++;
        return;
      }
      else if (NumPlanes > 4)
        memset(PixelBuffers[4], 0, header->cupsWidth * 2);
    }
    else
    {
     /*
      * Dither the pixels...
      */

      switch (NumPlanes)
      {
	  case 1 :
              dither_black(y, Dither, PixelBuffers[0]);
	      break;
	  case 3 :
              dither_cmyk(y, Dither, PixelBuffers[0], NULL, PixelBuffers[1],
	                  NULL, PixelBuffers[2], NULL, NULL);
	      break;
	  case 4 :
              dither_cmyk(y, Dither, PixelBuffers[1], NULL, PixelBuffers[2],
	                  NULL, PixelBuffers[3], NULL, PixelBuffers[0]);
	      break;
	  case 6 :
              dither_cmyk(y, Dither, PixelBuffers[1], PixelBuffers[4],
	                  PixelBuffers[2], PixelBuffers[5], PixelBuffers[3],
			  NULL, PixelBuffers[0]);
	      break;
      }
    }

   /*
    * Convert them to bits...
    */

    if (DotRowStep == 1)
    {
     /*
      * Handle microweaved output...
      */

      for (plane = 0; plane < NumPlanes; plane ++)
	PackHorizontal(PixelBuffers[plane], DotBuffers[0][plane], width, 0, 1);

      OutputLine(ppd, header, y);
    }
    else
    {
     /*
      * Handle softweaved output...
      */

      for (pass = 0; pass < DotColStep; pass ++, subrow += DotRowStep)
      {
       /*
	* See if we need to output the band...
	*/

	if ((y - DotRow[subrow]) >= DotRowMax)
	  OutputBand(ppd, header, subrow, y);

	offset = ((y - DotRow[subrow]) / DotRowStep) * length;

	for (plane = 0; plane < NumPlanes; plane ++)
          if (BitPlanes == 1)
	    PackHorizontal(PixelBuffers[plane] + pass,
	                   DotBuffers[subrow][plane] + offset,
                	   width, 0, DotColStep);
          else
	    PackHorizontal2(PixelBuffers[plane] + pass,
	                    DotBuffers[subrow][plane] + offset,
                	    width, DotColStep);

	DotBufferClear[subrow] = 0;
      }
    }
  }
}


/*
 * 'main()' - Main entry and processing of driver.
 */

int			/* O - Exit status */
main(int  argc,		/* I - Number of command-line arguments */
     char *argv[])	/* I - Command-line arguments */
{
  int			fd;	/* File descriptor */
  cups_raster_t		*ras;	/* Raster stream for printing */
  cups_page_header_t	header;	/* Page header from file */
  ppd_file_t		*ppd;	/* PPD file */
  int			page;	/* Current page */
  int			y;	/* Current line */


 /*
  * Check for valid arguments...
  */

  if (argc < 6 || argc > 7)
  {
   /*
    * We don't have the correct number of arguments; write an error message
    * and return.
    */

    fputs("ERROR: rastertoepson job-id user title copies options [file]\n", stderr);
    return (1);
  }

 /*
  * Open the page stream...
  */

  if (argc == 7)
  {
    if ((fd = open(argv[6], O_RDONLY)) == -1)
    {
      perror("ERROR: Unable to open raster file - ");
      sleep(1);
      return (1);
    }
  }
  else
    fd = 0;

  ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

 /*
  * Initialize the print device...
  */

  ppd = ppdOpenFile(getenv("PPD"));

  Setup();

 /*
  * Process pages as needed...
  */

  page = 0;

  while (cupsRasterReadHeader(ras, &header))
  {
   /*
    * Write a status message with the page number and number of copies.
    */

    page ++;

    fprintf(stderr, "PAGE: %d %d\n", page, header.NumCopies);

   /*
    * Start the page...
    */

    StartPage(ppd, &header);

   /*
    * Loop for each line on the page...
    */

    for (y = 0; y < header.cupsHeight; y ++)
    {
     /*
      * Let the user know how far we have progressed...
      */

      if ((y & 127) == 0)
        fprintf(stderr, "INFO: Printing page %d, %d%% complete...\n", page,
	        100 * y / header.cupsHeight);

     /*
      * Read and process a line of graphics...
      */

      ProcessLine(ppd, ras, &header, y);
    }

    fprintf(stderr, "INFO: Finished page %d...\n", page);

   /*
    * Eject the page...
    */

    EndPage(ppd, &header);
  }

 /*
  * Shutdown the printer...
  */

  Shutdown();

  ppdClose(ppd);

 /*
  * Close the raster stream...
  */

  cupsRasterClose(ras);
  if (fd != 0)
    close(fd);

 /*
  * If no pages were printed, send an error message...
  */

  if (page == 0)
    fputs("ERROR: No pages found!\n", stderr);
  else
    fputs("INFO: Ready to print.\n", stderr);

  return (page == 0);
}


/*
 * End of "$Id$".
 */
