#include <stdlib.h>
#include <stdio.h>


typedef struct			/* Weave parameters for a specific row */
{
  int row;			/* Absolute row # */
  int pass;			/* Computed pass # */
  int jet;			/* Which physical nozzle we're using */
  int missingstartrows;		/* Phantom rows (nonexistent rows that */
				/* would be printed by nozzles lower than */
				/* the first nozzle we're using this pass; */
				/* with the current algorithm, always zero */
  int logicalpassstart;		/* Offset in rows (from start of image) */
				/* that the printer must be for this row */
				/* to print correctly with the specified jet */
  int physpassstart;		/* Offset in rows to the first row printed */
				/* in this pass.  Currently always equal to */
				/* logicalpassstart */
  int physpassend;		/* Offset in rows (from start of image) to */
				/* the last row that will be printed this */
				/* pass (assuming that we're printing a full */
				/* pass). */
} weave_t;

typedef struct			/* Weave parameters for a specific pass */
{
  int pass;			/* Absolute pass number */
  int missingstartrows;		/* All other values the same as weave_t */
  int logicalpassstart;
  int physpassstart;
  int physpassend;
} pass_t;

typedef union {			/* Offsets from the start of each line */
  off_t v[6];			/* (really pass) */
  struct {
    off_t k;
    off_t m;
    off_t c;
    off_t y;
    off_t M;
    off_t C;
  } p;
} lineoff_t;

typedef union {			/* Base pointers for each pass */
  unsigned char *v[6];
  struct {
    unsigned char *k;
    unsigned char *m;
    unsigned char *c;
    unsigned char *y;
    unsigned char *M;
    unsigned char *C;
  } p;
} linebufs_t;

static unsigned char *linebufs;	/* Actual data buffers */
static linebufs_t *linebases;	/* Base address of each row buffer */
static lineoff_t *lineoffsets;	/* Offsets within each row buffer */
static int *linecounts;		/* How many rows we've printed this pass */
static pass_t *passes;		/* Circular list of pass numbers */
static int last_pass_offset;	/* Starting row (offset from the start of */
				/* the image) of the most recently printed */
				/* pass (so we can determine how far to */
				/* advance the paper) */
static int last_pass;		/* Number of the most recently printed pass */

static int njets;		/* Number of jets in use */
static int separation;		/* Separation between jets */

static int weavefactor;		/* Interleave factor (jets / separation) */
static int jetsused;		/* How many jets we can actually use */
static int initialoffset;	/* Distance between the first row we're */
				/* printing and the logical first row */
				/* (first nozzle of the first pass). */
				/* Currently this is zero. */
static int jetsleftover;	/* How many jets we're *not* using. */
				/* This can be used to rotate exactly */
				/* what jets we're using.  Currently this */
				/* is not used. */
static int weavespan;		/* How many rows total are bracketed by */
				/* one pass (separation * (jets - 1) */
static int horizontal_weave;	/* Number of horizontal passes required */
				/* This is > 1 for some of the ultra-high */
				/* resolution modes */

/*
 * Mapping between color and linear index.  The colors are
 * black, magenta, cyan, yellow, light magenta, light cyan
 */

static int color_indices[16] = { 0, 1, 2, -1,
				 3, -1, -1, -1,
				 -1, 4, 5, -1,
				 -1, -1, -1, -1 };
static int colors[6] = { 0, 1, 2, 4, 1, 2 };
static int densities[6] = { 0, 0, 0, 0, 1, 1 };

static int
get_color_by_params(int plane, int density)
{
  if (plane > 4 || plane < 0 || density > 1 || density < 0)
    return -1;
  return color_indices[density * 8 + plane];
}

/*
 * Initialize the weave parameters
 */
static void
initialize_weave(int jets, int sep, int horizontal_passes)
{
  int i;
  int k;
  char *bufbase;
  if (jets <= 1)
    separation = 1;
  if (sep <= 0)
    separation = 1;
  else
    separation = sep;
  njets = jets;
  if (horizontal_passes <= 0)
    horizontal_passes = 1;
  horizontal_weave = horizontal_passes;

  weavefactor = jets / separation;
  jetsused = ((weavefactor) * separation);
  initialoffset = (jetsused - weavefactor - 1) * separation;
  jetsleftover = njets - jetsused + 1;
  weavespan = (jetsused - 1) * separation;

  last_pass_offset = 0;
  last_pass = -1;

  linebufs = malloc(6 * 1536 * separation * jetsused * horizontal_passes);
  lineoffsets = malloc(separation * sizeof(lineoff_t) * horizontal_passes);
  linebases = malloc(separation * sizeof(linebufs_t) * horizontal_passes);
  passes = malloc(separation * sizeof(pass_t));
  linecounts = malloc(separation * sizeof(int));

  bufbase = linebufs;
  
  for (i = 0; i < separation; i++)
    {
      int j;
      passes[i].pass = -1;
      for (k = 0; k < horizontal_weave; k++)
	{
	  for (j = 0; j < 6; j++)
	    {
	      linebases[i * horizontal_weave + k].v[j] = bufbase;
	      bufbase += 1536 * jetsused;
	    }
	}
    }
}


static lineoff_t *
get_lineoffsets(int row)
{
  return &(lineoffsets[horizontal_weave * (row % separation)]);
}

static int *
get_linecount(int row)
{
  return &(linecounts[row % separation]);
}

static const linebufs_t *
get_linebases(int row)
{
  return &(linebases[horizontal_weave * (row % separation)]);
}

static pass_t *
get_pass(int row_or_pass)
{
  return &(passes[row_or_pass % separation]);
}

/*
 * Compute the weave parameters for the given row.  This computation is
 * rather complex, and I need to go back and write down very carefully
 * what's going on here.
 */

static void
weave_parameters_by_row(int row, weave_t *w)
{
  int passblockstart = (row + initialoffset) / jetsused;
  int internaljetsused = jetsused;
  int internallogicalpassstart;
  w->row = row;
  w->pass = (passblockstart - (separation - 1)) +
    (separation + row - passblockstart - 1) % separation;
  internallogicalpassstart = (w->pass * jetsused) - initialoffset +
    (w->pass % separation);
  if (internallogicalpassstart < 0)
    {
      internaljetsused -=
	(((separation - 1) - internallogicalpassstart) / separation);
      internallogicalpassstart += separation *
	(((separation - 1) - internallogicalpassstart) / separation);
    }
  w->logicalpassstart =  internallogicalpassstart;
  w->jet = ((row - w->logicalpassstart) / separation);
  if (internallogicalpassstart >= 0)
    w->physpassstart = internallogicalpassstart;
  else
    w->physpassstart = internallogicalpassstart +
      (separation * ((separation - internallogicalpassstart) / separation));
  w->physpassend = (internaljetsused - 1) * separation +
    internallogicalpassstart;
  w->missingstartrows = (w->physpassstart - w->logicalpassstart) / separation;
}

int passstarts[1000];
int passends[1000];
int passcounts[1000];

int
main(int argc, char **argv)
{
  int i;
  weave_t w;
  int lastpass = -1;
  int newestpass = -1;
  initialize_weave(16, 8, 2);
  printf("%8s %5s %5s %5s %10s %10s %10s %10s\n", "", "row", "pass", "jet",
	 "missing", "logical", "physstart", "physend");
  for (i = 0; i < 1000; i++)
    {
      passstarts[i] = -1;
      passends[i] = -1;
    }
  for (i = 0; i < 1000; i++)
    {
      int physrow;
      weave_parameters_by_row(i, &w);
      physrow = w.logicalpassstart + 8 * w.jet;
      printf("%c%c%c%c%c%c%c%c%5d %5d %5d %10d %10d %10d %10d\n",
	     w.pass < 0 ? 'A' : ' ',
	     w.jet < 0 || w.jet > 31 ? 'B' : ' ',
	     w.physpassstart > w.row ? 'C' : ' ',
	     w.physpassend < w.row ? 'D' : ' ',
	     w.physpassend == w.row && lastpass + 1 != w.pass ? 'E' : ' ',
	     passstarts[w.pass] != -1 && passstarts[w.pass] !=w.physpassstart ?
	     'F' : ' ',
	     passends[w.pass] != -1 && passends[w.pass] !=w.physpassend ?
	     'G' : ' ',
	     w.row != physrow ? 'H' : ' ',
	     w.row, w.pass, w.jet,
	     w.missingstartrows, w.logicalpassstart, w.physpassstart,
	     w.physpassend);
      if (w.physpassend == w.row)
	{
	  lastpass = w.pass;
	  passends[w.pass] = -2;
	}
      else
	passends[w.pass] = w.physpassend;
      passstarts[w.pass] = w.physpassstart;
      if (w.pass > newestpass)
	newestpass = w.pass;
    }
  printf("Unterminated passes:\n");
  for (i = 0; i <= newestpass; i++)
    if (passends[i] >= -1 && passends[i] < 1000)
      printf("%d %d\n", i, passends[i]);
  printf("Last terminated pass: %d\n", lastpass);
  return 0;
}
