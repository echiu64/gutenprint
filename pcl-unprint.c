/*
 * "$Id$"
 *
 * pclunprint.c - convert an HP PCL file into an image file for viewing.
 *
 * Dave Hill <dave@minnie.demon.co.uk>
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
 * Revision History:
 *	(at bottom)
 */

#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>

static char *id="@(#) $Id$";

/* 
 * Largest data attached to a command. 1024 means that we can have up to 8192
 * pixels in a row
 */
#define MAX_DATA 1024

FILE *read_fd,*write_fd;
char read_buffer[1024];
char data_buffer[MAX_DATA];
char initial_command[10];
int initial_command_index;
char final_command;
int numeric_arg;

int read_pointer;
int read_size;
int eof;

/*
 * Data about the image
 */

typedef struct {
    int colour_type;		/* Mono, 3/4 colour */
    int black_depth;		/* 2 level, 4 level */
    int cyan_depth;		/* 2 level, 4 level */
    int magenta_depth;		/* 2 level, 4 level */
    int yellow_depth;		/* 2 level, 4 level */
    int image_width;
    int image_height;
    int compression_type;	/* Uncompressed or TIFF */
} image_t;

/*
 * collected data read from file
 */

typedef struct {
    char **black_bufs;			/* Storage for black rows */
    int black_data_rows_per_row;	/* Number of black rows */
    char **cyan_bufs;
    int cyan_data_rows_per_row;
    char **magenta_bufs;
    int magenta_data_rows_per_row;
    char **yellow_bufs;
    int yellow_data_rows_per_row;
    int active_length;			/* Length of output data */
    int output_depth;
} output_t;

#define PCL_MONO 1
#define PCL_CMY 3
#define PCL_CMYK 4

#define PCL_COMPRESSION_NONE 0
#define PCL_COMPRESSION_TIFF 2

/* PCL COMMANDS */

#define PCL_RESET 1
#define PCL_MEDIA_SIZE 2
#define PCL_PERF_SKIP 3
#define PCL_TOP_MARGIN 4
#define PCL_MEDIA_TYPE 5
#define PCL_MEDIA_SOURCE 6
#define PCL_SHINGLING 7
#define PCL_RASTERGRAPHICS_QUALITY 8
#define PCL_DEPLETION 9
#define PCL_CONFIGURE 10
#define PCL_RESOLUTION 11
#define PCL_COLOURTYPE 12
#define PCL_COMPRESSIONTYPE 13
#define PCL_LEFTRASTER_POS 14
#define PCL_TOPRASTER_POS 15
#define PCL_RASTER_WIDTH 16
#define PCL_RASTER_HEIGHT 17
#define PCL_START_RASTER 18
#define PCL_END_RASTER 19
#define PCL_END_RASTER_NEW 20
#define PCL_DATA 21
#define PCL_DATA_LAST 22
#define PCL_PRINT_QUALITY 23
#define PCL_PJL_COMMAND 24
#define PCL_UNK1 25
#define PCL_UNK2 26
#define PCL_PAGE_ORIENTATION 27
#define PCL_VERTICAL_CURSOR_POSITIONING_BY_DOTS 28
#define PCL_HORIZONTAL_CURSOR_POSITIONING_BY_DOTS 29
#define PCL_UNK3 30
#define PCL_RELATIVE_VERTICAL_PIXEL_MOVEMENT 31
#define PCL_PALETTE_CONFIGURATION 32

typedef struct{
    char *initial_command;		/* First part of command */
    char final_command;			/* Last part of command */
    int has_data;			/* Data follows */
    int command;			/* Command name */
    char *description;			/* Text for printing */
} commands_t;

commands_t pcl_commands[] =
    {
        { "E", '\0', 0, PCL_RESET, "PCL RESET" },
	{ "&l", 'A', 0, PCL_MEDIA_SIZE , "Media Size" },
	{ "&l", 'L', 0, PCL_PERF_SKIP , "Perf. Skip" },
	{ "&l", 'E', 0, PCL_TOP_MARGIN , "Top Margin" },
	{ "&l", 'M', 0, PCL_MEDIA_TYPE , "Media Type" },
	{ "&l", 'H', 0, PCL_MEDIA_SOURCE, "Media Source" },
	{ "*o", 'Q', 0, PCL_SHINGLING, "Raster Graphics Shingling" },
	{ "*r", 'Q', 0, PCL_RASTERGRAPHICS_QUALITY, "Raster Graphics Quality" },
	{ "*o", 'D', 0, PCL_DEPLETION, "Depletion" },
	{ "*g", 'W', 1, PCL_CONFIGURE, "Extended Configure" },
	{ "*t", 'R', 0, PCL_RESOLUTION, "Resolution" },
	{ "*r", 'U', 0, PCL_COLOURTYPE, "Colour Type" },
	{ "*b", 'M', 0, PCL_COMPRESSIONTYPE, "Compression Type" },
	{ "&a", 'H', 0, PCL_LEFTRASTER_POS, "Left Raster Position" },
	{ "&a", 'V', 0, PCL_TOPRASTER_POS, "Top Raster Position" },
	{ "*r", 'S', 0, PCL_RASTER_WIDTH, "Raster Width" },
	{ "*r", 'T', 0, PCL_RASTER_HEIGHT, "Raster Height" },
	{ "*r", 'A', 0, PCL_START_RASTER, "Start Raster Graphics" },
	{ "*rB", '\0', 0, PCL_END_RASTER, "End Raster Graphics"},
	{ "*rC", '\0', 0, PCL_END_RASTER_NEW, "End Raster Graphics" },
	{ "*b", 'V', 1, PCL_DATA, "Data, intermediate" },
	{ "*b", 'W', 1, PCL_DATA_LAST, "Data, last" },
	{ "*o", 'M', 0, PCL_PRINT_QUALITY, "Print Quality" },
	{ "%", 'X', 0, PCL_PJL_COMMAND, "PJL Command" },	/* Special! */
	{ "*b", 'B', 0, PCL_UNK1, "Unknown 1" },
	{ "*o", 'W', 1, PCL_UNK2, "Unknown 2" },
	{ "&l", 'O', 0, PCL_PAGE_ORIENTATION, "Page Orientation" },
	{ "*p", 'Y', 0, PCL_VERTICAL_CURSOR_POSITIONING_BY_DOTS, "Vertical Cursor Positioning by Dots" },
	{ "*p", 'X', 0, PCL_HORIZONTAL_CURSOR_POSITIONING_BY_DOTS, "Horizontal Cursor Positioning by Dots" },
	{ "&u", 'D', 0, PCL_UNK3, "Unknown 3" },
	{ "*b", 'Y', 0, PCL_RELATIVE_VERTICAL_PIXEL_MOVEMENT, "Relative Vertical Pixel Movement" },
	{ "*d", 'W', 1, PCL_PALETTE_CONFIGURATION, "Palette Configuration" },
   };

/*
 * pcl_find_command(). Search the commands table for the command.
 */

int pcl_find_command() {

    int num_commands = sizeof(pcl_commands) / sizeof(commands_t);
    int i;

    for (i=0; i < num_commands; i++) {
        if ((strcmp(initial_command, pcl_commands[i].initial_command) == 0) &&
	    (final_command == pcl_commands[i].final_command))
		return(i);
    }

    return (-1);
}

/*
 * fill_buffer() - Read a new chunk from the input file
 */

void fill_buffer() {

    if ((read_pointer == -1) || (read_pointer >= read_size)) {
        read_size = (int) fread(&read_buffer, sizeof(char), 1024, read_fd);

#ifdef DEBUG
        fprintf(stderr, "Read %d characters\n", read_size);
#endif

        if (read_size == 0) {
#ifdef DEBUG
            fprintf(stderr, "No More to read!\n");
#endif
            eof = 1;
            return;
        }
        read_pointer = 0;
    }
}

/*
 * pcl_read_command() - Read the data stream and parse the next PCL
 * command.
 */

void pcl_read_command() {

    char c;
    int minus;
    int skip_prefix;

/* 
   The format of a PCL command is: ESC <x> <y> [n] <z>
   where x, y and z are characters, and [n] is an optional number.
   Some commands are followed by data, in this case, [n] is the
   number of bytes to read.
   An exception is "ESC E" (reset), also "ESC %-12345X<command>\n"

   The fun is that it is possible to abbreviate commands if the
   command prefix is the same, e.g. ESC & 26 A 0L is the same as
   ESC & 26 A ESC 0 L
*/

    numeric_arg=0;
    minus = 0;
    final_command = '\0';
    skip_prefix = 0;		/* Process normally */

    fill_buffer();
    if (eof == 1)
        return;

/* First character must be ESC, otherwise we have gone wrong! */

    c = read_buffer[read_pointer];
    read_pointer++;
#ifdef DEBUG
    fprintf(stderr, "Got %c\n", c);
#endif
    if(c != (char) 0x1b) {

/*
 * If this is a digit, or minus, and the last command was valid,
 * then assume that this command shares the same prefix. Of
 * course an unknown command followed by data followed by
 * a repeated command will fool us completely!
 */

	if ((isdigit(c) || (c == '-')) && (initial_command_index != 0)) {
#ifdef DEBUG
	    fprintf(stderr, "Possible start of repeated command.\n");
#endif
	    skip_prefix = 1;
        }
	else {
	    fprintf(stderr, "ERROR: No ESC found (out of sync?) searching...\n");
/*
 * all we can do is to chew through the file looking for another ESC.
 */

	    while (c != (char) 0x1b) {
		fill_buffer();
		if (eof == 1) {
		    fprintf(stderr, "ERROR: EOF looking for ESC!\n");
		    return;
		}
		c = read_buffer[read_pointer];
		read_pointer++;
	    }
	}
    }

    if (skip_prefix == 0) {

/*
 * We got an ESC, process normally
 */

	initial_command_index=0;
	initial_command[initial_command_index] = '\0';
	fill_buffer();
	if (eof == 1) {
	    fprintf(stderr, "ERROR: EOF after ESC!\n");
	    return;
	}

/* Get first command letter */

	c = read_buffer[read_pointer];
	initial_command[initial_command_index] = c;

#ifdef DEBUG
	fprintf(stderr, "Got %c\n", c);
#endif

	read_pointer++;

	initial_command_index++;

/* Now keep going until we find a numeric, or another ESC, or EOF */

	while (1) {
	    fill_buffer();
	    if (eof == 1) {

#ifdef DEBUG
		fprintf(stderr, "EOF in middle of command!\n");
#endif
		eof = 0;		/* Ignore it */
		initial_command[initial_command_index] = '\0';
		return;
	    }
	    c = read_buffer[read_pointer];
	    if (c == (char) 0x1b) {

#ifdef DEBUG
		fprintf(stderr, "Got another ESC!\n");
#endif

		initial_command[initial_command_index] = '\0';
		return;
	    }
	    if (iscntrl(c) != 0) {

#ifdef DEBUG
		fprintf(stderr, "Got a control char!\n");
#endif

		initial_command[initial_command_index] = '\0';
		return;
	    }

#ifdef DEBUG
	    fprintf(stderr, "Got %c\n", c);
#endif

	    read_pointer++;
	    if ((isdigit(c)) || (c == '-'))
		break;
	    initial_command[initial_command_index] = c;
	    initial_command_index++;
	}

	initial_command[initial_command_index] = '\0';
    }

    if (c == '-')
	minus = 1;
    else
	numeric_arg = (int) (c - '0');

    while (1) {
        fill_buffer();
        if (eof == 1) {
            fprintf(stderr, "ERROR: EOF in middle of command!\n");
            return;
        }
        c = read_buffer[read_pointer];

#ifdef DEBUG
        fprintf(stderr, "Got %c\n", c);
#endif

        read_pointer++;
        if (! isdigit(c)) {
            final_command = toupper(c);
	    if (minus == 1)
		numeric_arg = -numeric_arg;
            return;
        }
        numeric_arg = (10 * numeric_arg) + (int) (c - '0');
    }
}

/*
 * write_grey() - write out one line of mono PNM image
 */

/* FIXME - multiple levels */

void write_grey(output_t *output,	/* I: data */
		image_t *image)		/* I: Image data */
{
    int wholebytes = image->image_width / 8;
    int crumbs = image->image_width - (wholebytes * 8);
    char *buf = output->black_bufs[0];

    int i, j;
    char tb[8];

#ifdef DEBUG
    fprintf(stderr, "Data Length: %d, wholebytes: %d, crumbs: %d\n",
	output->active_length, wholebytes, crumbs);
#endif

    for (i=0; i < wholebytes; i++) {
	for (j=0; j < 8; j++) {
	    tb[j] = (((buf[i] >> (7-j)) & 1));
	    tb[j] = output->output_depth - tb[j];
	}
	(void) fwrite(&tb[0], sizeof(char), 8, write_fd);
    }
    for (j=0; j < crumbs; j++) {
	tb[j] = (((buf[wholebytes] >> (7-j)) & 1));
	tb[j] = output->output_depth - tb[j];
    }
    (void) fwrite(&tb[0], sizeof(char), (size_t) crumbs, write_fd);
}

/*
 * write_colour() - Write out one row of RGB PNM data.
 */

/* FIXME - multiple levels and CMYK */

void write_colour(output_t *output,		/* I: Data buffers */
		 image_t *image)		/* I: Image data */
{
    int wholebytes = image->image_width / 8;
    int crumbs = image->image_width - (wholebytes * 8);

    int i, j, jj;
    char tb[8*3];

    char *cyan_buf;
    char *magenta_buf;
    char *yellow_buf;
    char *black_buf;

    cyan_buf = output->cyan_bufs[0];
    magenta_buf = output->magenta_bufs[0];
    yellow_buf = output->yellow_bufs[0];
    if (image->colour_type == PCL_CMYK)
	black_buf = output->black_bufs[0];
    else
	black_buf = NULL;

#ifdef DEBUG
    fprintf(stderr, "Data Length: %d, wholebytes: %d, crumbs: %d, planes: %d\n",
	output->active_length, wholebytes, crumbs, image->colour_type);

    fprintf(stderr, "Cyan: ");
    for (i=0; i < output->active_length; i++) {
	fprintf(stderr, "%02x ", (unsigned char) cyan_buf[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Magenta: ");
    for (i=0; i < output->active_length; i++) {
	fprintf(stderr, "%02x ", (unsigned char) magenta_buf[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Yellow: ");
    for (i=0; i < output->active_length; i++) {
	fprintf(stderr, "%02x ", (unsigned char) yellow_buf[i]);
    }
    fprintf(stderr, "\n");
    if (image->colour_type == PCL_CMYK) {
	fprintf(stderr, "Black: ");
	for (i=0; i < output->active_length; i++) {
	    fprintf(stderr, "%02x ", (unsigned char) black_buf[i]);
	}
	fprintf(stderr, "\n");
    }
#endif

    if (image->colour_type != PCL_CMYK) {
	for (i=0; i < wholebytes; i++) {
	    for (j=0,jj=0; j < 8; j++,jj+=3) {
		tb[jj] = (((cyan_buf[i] >> (7-j)) & 1));
		tb[jj] = output->output_depth - tb[jj];
		tb[jj+1] = (((magenta_buf[i] >> (7-j)) & 1));
		tb[jj+1] = output->output_depth - tb[jj+1];
		tb[jj+2] = (((yellow_buf[i] >> (7-j)) & 1));
		tb[jj+2] = output->output_depth - tb[jj+2];
	    }
	    (void) fwrite(&tb[0], sizeof(char), (size_t) (8*3), write_fd);
	}
	for (j=0,jj=0; j < crumbs; j++,jj+=3) {
	    tb[jj] = (((cyan_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj] = output->output_depth - tb[jj];
	    tb[jj+1] = (((magenta_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+1] = output->output_depth - tb[jj+1];
	    tb[jj+2] = (((yellow_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+2] = output->output_depth - tb[jj+2];
	}
	(void) fwrite(&tb[0], sizeof(char), (size_t) crumbs*3, write_fd);
    }
    else {
	for (i=0; i < wholebytes; i++) {
	    for (j=0,jj=0; j < 8; j++,jj+=3) {
#if !defined OUTPUT_CMYK_ONLY_K && !defined OUTPUT_CMYK_ONLY_CMY
		tb[jj] = ((((cyan_buf[i]|black_buf[i]) >> (7-j)) & 1));
		tb[jj+1] = ((((magenta_buf[i]|black_buf[i]) >> (7-j)) & 1));
		tb[jj+2] = ((((yellow_buf[i]|black_buf[i]) >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_K
		tb[jj] = (((black_buf[i] >> (7-j)) & 1));
		tb[jj+1] = (((black_buf[i] >> (7-j)) & 1));
		tb[jj+2] = (((black_buf[i] >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_CMY
		tb[jj] = (((cyan_buf[i] >> (7-j)) & 1));
		tb[jj+1] = (((magenta_buf[i] >> (7-j)) & 1));
		tb[jj+2] = (((yellow_buf[i] >> (7-j)) & 1));
#endif
		tb[jj] = output->output_depth - tb[jj];
		tb[jj+1] = output->output_depth - tb[jj+1];
		tb[jj+2] = output->output_depth - tb[jj+2];
	    }
	    (void) fwrite(&tb[0], sizeof(char), (size_t) (8*3), write_fd);
	}
	for (j=0,jj=0; j < crumbs; j++,jj+=3) {
#if !defined OUTPUT_CMYK_ONLY_K && !defined OUTPUT_CMYK_ONLY_CMY
	    tb[jj] = ((((cyan_buf[wholebytes]|black_buf[wholebytes]) >> (7-j)) & 1));
	    tb[jj+1] = ((((magenta_buf[wholebytes]|black_buf[wholebytes]) >> (7-j)) & 1));
	    tb[jj+2] = ((((yellow_buf[wholebytes]|black_buf[wholebytes]) >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_K
	    tb[jj] = (((black_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+1] = (((black_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+2] = (((black_buf[wholebytes] >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_CMY
	    tb[jj] = (((cyan_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+1] = (((magenta_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+2] = (((yellow_buf[wholebytes] >> (7-j)) & 1));
#endif
	    tb[jj] = output->output_depth - tb[jj];
	    tb[jj+1] = output->output_depth - tb[jj+1];
	    tb[jj+2] = output->output_depth - tb[jj+2];
	}
	(void) fwrite(&tb[0], sizeof(char), (size_t) crumbs*3, write_fd);
    }
}

/*
 * decode_tiff() - Uncompress a TIFF encoded buffer
 */

int decode_tiff(char *in_buffer,		/* I: Data buffer */
		int data_length,		/* I: Length of data */
		char *decode_buf,		/* O: decoded data */
		int maxlen)			/* I: Max length of decode_buf */
{	
/* The TIFF coding consists of either:-
 *
 * (0 <= count <= 127) (count+1 bytes of data) for non repeating data
 * or
 * (-127 <= count <= -1) (data) for 1-count bytes of repeating data
 */

    int count;
    int pos = 0;
    int dpos = 0;
#ifdef DEBUG
    int i;
#endif

    while(pos < data_length ) {

	count = in_buffer[pos];

	if ((count >= 0) && (count <= 127)) {
#ifdef DEBUG
	    fprintf(stderr, "%d bytes of nonrepeated data\n", count+1);
	    fprintf(stderr, "DATA: ");
	    for (i=0; i< (count+1); i++) {
		fprintf(stderr, "%02x ", (unsigned char) in_buffer[pos + 1 + i]);
	    }
	    fprintf(stderr, "\n");
#endif
	    if ((dpos + count + 1) > maxlen) {
		fprintf(stderr, "ERROR: Too much expanded data (%d), increase MAX_DATA!\n", dpos + count + 1);
		exit(EXIT_FAILURE);
	    }
	    memcpy(&decode_buf[dpos], &in_buffer[pos+1], (size_t) (count + 1));
	    dpos += count + 1;
	    pos += count + 2;
	}
	else if ((count >= -127) && (count < 0)) {
#ifdef DEBUG
	    fprintf(stderr, "%02x repeated %d times\n", (unsigned char) in_buffer[pos + 1], 1 - count);
#endif
	    if ((dpos + 1 - count) > maxlen) {
		fprintf(stderr, "ERROR: Too much expanded data (%d), increase MAX_DATA!\n", dpos + 1 - count);
		exit(EXIT_FAILURE);
	    }
	    memset(&decode_buf[dpos], in_buffer[pos + 1], (size_t) (1 - count));
	    dpos += 1 - count;
	    pos += 2;
	}
	else {
	    fprintf(stderr, "ERROR: Illegal TIFF count: %d, skipped\n", count);
	    pos += 2;
	}
    }

#ifdef DEBUG
    fprintf(stderr, "TIFFOUT: ");
    for (i=0; i< dpos; i++) {
	fprintf(stderr, "%02x ", (unsigned char) decode_buf[i]);
    }
    fprintf(stderr, "\n");
#endif
    return(dpos);
}

/*
 * pcl_reset() - Rest image parameters to default
 */

void pcl_reset(image_t *i) {
    i->colour_type = PCL_MONO;
    i->black_depth = 2;		/* Assume mono */
    i->cyan_depth = 0;
    i->magenta_depth = 0;
    i->yellow_depth = 0;
    i->image_width = -1;
    i->image_height = -1;
    i->compression_type = 0;	/* should this be NONE? */
}

/*
 * depth_to_rows() - convert the depth of the colour into the number
 * of data rows needed to represent it. Assumes that depth is a power
 * of 2, FIXME if not!
 */

int depth_to_rows(int depth){
    int rows;

    if (depth == 0)
	return(0);

    for (rows = 1; rows < 8; rows++) {
    if ((depth >> rows) == 1)
	return(rows);
    }
    fprintf(stderr, "ERROR: depth %d too big to handle in depth_to_rows()!\n",
	depth);
    return(0);	/* ?? */
}

/*
 * Main
 */

int main(int argc, char *argv[]) {

    int command_index;
    int command;
    int i, j;				/* Loop/general variables */
    int image_row_counter = -1;		/* Count of current row */
    int current_data_row = -1;		/* Count of data rows received for this output row */
    int expected_data_rows_per_row = -1;
					/* Expected no of data rows per output row */
    image_t image_data;			/* Data concerning image */
    long filepos = -1;

/*
 * Holders for the decoded lines
 */

    output_t output_data;

/*
 * The above pointers (when allocated) are then copied into this
 * variable in the correct order so that the received data can
 * be stored.
 */

    char **received_rows;

    output_data.black_bufs = NULL;		/* Storage for black rows */
    output_data.black_data_rows_per_row = 0;	/* Number of black rows */
    output_data.cyan_bufs = NULL;
    output_data.cyan_data_rows_per_row = 0;
    output_data.magenta_bufs = NULL;
    output_data.magenta_data_rows_per_row = 0;
    output_data.yellow_bufs = NULL;
    output_data.yellow_data_rows_per_row = 0;
    output_data.active_length = 0;

    received_rows = NULL;

    if(argc == 1){
        read_fd = stdin;
        write_fd = stdout;
    }
    else if(argc == 2){
        read_fd = fopen(argv[1],"r");
        write_fd = stdout;
    }
    else {
        if(*argv[1] == '-'){
            read_fd = stdin;
            write_fd = fopen(argv[2],"w");
        }
        else {
            read_fd = fopen(argv[1],"r");
            write_fd = fopen(argv[2],"w");
        }
    }

    if (read_fd == (FILE *)NULL) {
        fprintf(stderr, "ERROR: Error Opening input file.\n");
        exit (EXIT_FAILURE);
    }

    if (write_fd == (FILE *)NULL) {
        fprintf(stderr, "ERROR: Error Opening output file.\n");
        exit (EXIT_FAILURE);
    }

    read_pointer=-1;
    eof=0;
    initial_command_index=0;
    initial_command[initial_command_index] = '\0';
    numeric_arg=0;
    final_command = '\0';


    pcl_reset(&image_data);

    while (1) {
        pcl_read_command();
        if (eof == 1) {
#ifdef DEBUG
            fprintf(stderr, "EOF while reading command.\n");
#endif
            (void) fclose(read_fd);
            (void) fclose(write_fd);
            exit(EXIT_SUCCESS);
        }

#ifdef DEBUG
        fprintf(stderr, "initial_command: %s, numeric_arg: %d, final_command: %c\n",
            initial_command, numeric_arg, final_command);
#endif

	command_index = pcl_find_command();
        if (command_index == -1) {
            fprintf(stderr, "ERROR: Unknown (and unhandled) command: %s%d%c\n", initial_command,
                numeric_arg, final_command);
/* We may have to skip some data here */
	}
	else {
	    command = pcl_commands[command_index].command;
	    if (pcl_commands[command_index].has_data == 1) {

/* Read the data into data_buffer */

#ifdef DEBUG
		fprintf(stderr, "Data: ");
#endif

		if (numeric_arg > MAX_DATA) {
		    fprintf(stderr, "ERROR: Too much data (%d), increase MAX_DATA!\n", numeric_arg);
		    exit(EXIT_FAILURE);
		}

		for (i=0; i < numeric_arg; i++) {
		    fill_buffer();
		    if (eof == 1) {
			fprintf(stderr, "ERROR: Unexpected EOF whilst reading data\n");
			exit(EXIT_FAILURE);
		    }
		    data_buffer[i] = read_buffer[read_pointer];

#ifdef DEBUG
		    fprintf(stderr, "%02x ", (unsigned char) data_buffer[i]);
#endif

		    read_pointer++;
		}

#ifdef DEBUG
		fprintf(stderr, "\n");
#endif

	    }
	    switch(command) {
	    case PCL_RESET :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);
		pcl_reset(&image_data);
		break;

	    case PCL_START_RASTER :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);

/* Make sure we have all the stuff needed to work out what we are going
   to write out. */

		i = 0;		/* use as error indicator */

		if (image_data.image_width == -1) {
		    fprintf(stderr, "ERROR: Image width not set!\n");
		    i++;
		}
		if (image_data.image_height == -1) {
		    fprintf(stderr, "ERROR: Image height not set!\n");
/*		    i++; */
		}

		if ((image_data.black_depth != 0) &&
		    (image_data.black_depth != 2)) {
		    fprintf(stderr, "Sorry, only 2 level black dithers handled.\n");
		    i++;
		}
		if ((image_data.cyan_depth != 0) &&
		    (image_data.cyan_depth != 2)) {
		    fprintf(stderr, "Sorry, only 2 level cyan dithers handled.\n");
		    i++;
		}
		if ((image_data.magenta_depth != 0) &&
		    (image_data.magenta_depth != 2)) {
		    fprintf(stderr, "Sorry, only 2 level magenta dithers handled.\n");
		    i++;
		}
		if ((image_data.yellow_depth != 0) &&
		    (image_data.yellow_depth != 2)) {
		    fprintf(stderr, "Sorry, only 2 level yellow dithers handled.\n");
		    i++;
		}

		if ((image_data.compression_type != PCL_COMPRESSION_NONE) &&
			(image_data.compression_type != PCL_COMPRESSION_TIFF)) {
		    fprintf(stderr,
			"Sorry, only 'no compression' or 'tiff compression' handled.\n");
		    i++;
		}

		if (i != 0) {
		    fprintf(stderr, "Cannot continue.\n");
		    exit (EXIT_FAILURE);
		}

		if (image_data.colour_type == PCL_MONO)
		    (void) fputs("P5\n", write_fd);	/* Raw, Grey */
		else
		    (void) fputs("P6\n", write_fd);	/* Raw, RGB */

		(void) fputs("# Written by pclunprint.\n", write_fd);

/*
 * Remember the file position where we wrote the image width and height
 * (you don't want to know why!)
 */

		filepos = ftell(write_fd);

		fprintf(write_fd, "%10d %10d\n", image_data.image_width,
		    image_data.image_height);
		
/*
 * Write the depth of the image
 */

		if (image_data.black_depth != 0)
		    output_data.output_depth = image_data.black_depth - 1;
		else
		    output_data.output_depth = image_data.cyan_depth - 1;
		fprintf(write_fd, "%d\n", output_data.output_depth);

		image_row_counter = 0;
		current_data_row = 0;

		output_data.black_data_rows_per_row = depth_to_rows(image_data.black_depth);
		output_data.cyan_data_rows_per_row = depth_to_rows(image_data.cyan_depth);
		output_data.magenta_data_rows_per_row = depth_to_rows(image_data.magenta_depth);
		output_data.yellow_data_rows_per_row = depth_to_rows(image_data.yellow_depth);

/*
 * Allocate some storage for the expected planes
 */

		if (output_data.black_data_rows_per_row != 0) {
		    output_data.black_bufs = malloc(output_data.black_data_rows_per_row * sizeof (char *));
		    for (i=0; i < output_data.black_data_rows_per_row; i++) {
			output_data.black_bufs[i] = malloc(MAX_DATA * sizeof (char));
		    }
		}
		if (output_data.cyan_data_rows_per_row != 0) {
		    output_data.cyan_bufs = malloc(output_data.cyan_data_rows_per_row * sizeof (char *));
		    for (i=0; i < output_data.cyan_data_rows_per_row; i++) {
			output_data.cyan_bufs[i] = malloc(MAX_DATA * sizeof (char));
		    }
		}
		if (output_data.magenta_data_rows_per_row != 0) {
		    output_data.magenta_bufs = malloc(output_data.magenta_data_rows_per_row * sizeof (char *));
		    for (i=0; i < output_data.magenta_data_rows_per_row; i++) {
			output_data.magenta_bufs[i] = malloc(MAX_DATA * sizeof (char));
		    }
		}
		if (output_data.yellow_data_rows_per_row != 0) {
		    output_data.yellow_bufs = malloc(output_data.yellow_data_rows_per_row * sizeof (char *));
		    for (i=0; i < output_data.yellow_data_rows_per_row; i++) {
			output_data.yellow_bufs[i] = malloc(MAX_DATA * sizeof (char));
		    }
		}

/*
 * Now store the pointers in the right order to make life easier in the
 * decoding phase
 */

		expected_data_rows_per_row = output_data.black_data_rows_per_row + 
		    output_data.cyan_data_rows_per_row + output_data.magenta_data_rows_per_row + 
		    output_data.yellow_data_rows_per_row;

		received_rows = malloc(expected_data_rows_per_row * sizeof(char *));
		j = 0;
		for (i = 0; i < output_data.black_data_rows_per_row; i++)
		    received_rows[j++] = output_data.black_bufs[i];
		for (i = 0; i < output_data.cyan_data_rows_per_row; i++)
		    received_rows[j++] = output_data.cyan_bufs[i];
		for (i = 0; i < output_data.magenta_data_rows_per_row; i++)
		    received_rows[j++] = output_data.magenta_bufs[i];
		for (i = 0; i < output_data.yellow_data_rows_per_row; i++)
		    received_rows[j++] = output_data.yellow_bufs[i];

		break;

	    case PCL_END_RASTER :
	    case PCL_END_RASTER_NEW :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);

/*
 * Check that we got the correct number of rows of data. If the expected number is
 * -1, invoke MAJOR BODGERY!
 */

		if (image_data.image_height == -1) {
		    image_data.image_height = image_row_counter;
		    if (fseek(write_fd, filepos, SEEK_SET) != -1) {
			fprintf(write_fd, "%10d %10d\n", image_data.image_width,
			    image_data.image_height);
			fseek(write_fd, 0L, SEEK_END);
		    }
		}

		if (image_row_counter != image_data.image_height)
		    fprintf(stderr, "ERROR: Row count mismatch. Expected %d rows, got %d rows.\n",
			image_data.image_height, image_row_counter);
		else
		    fprintf(stderr, "\t%d rows processed.\n", image_row_counter);

		if (output_data.black_data_rows_per_row != 0) {
		    for (i=0; i < output_data.black_data_rows_per_row; i++) {
			free(output_data.black_bufs[i]);
		    }
		    free(output_data.black_bufs);
		    output_data.black_bufs = NULL;
		}
		if (output_data.cyan_data_rows_per_row != 0) {
		    for (i=0; i < output_data.cyan_data_rows_per_row; i++) {
			free(output_data.cyan_bufs[i]);
		    }
		    free(output_data.cyan_bufs);
		    output_data.cyan_bufs = NULL;
		}
		if (output_data.magenta_data_rows_per_row != 0) {
		    for (i=0; i < output_data.magenta_data_rows_per_row; i++) {
			free(output_data.magenta_bufs[i]);
		    }
		    free(output_data.magenta_bufs);
		    output_data.magenta_bufs = NULL;
		}
		if (output_data.yellow_data_rows_per_row != 0) {
		    for (i=0; i < output_data.yellow_data_rows_per_row; i++) {
			free(output_data.yellow_bufs[i]);
		    }
		    free(output_data.yellow_bufs);
		    output_data.yellow_bufs = NULL;
		}
		free(received_rows);
		received_rows = NULL;

		break;

	    case PCL_MEDIA_SIZE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 2 :
		    fprintf(stderr, "Letter\n");
		    break;
		case 3 :
		    fprintf(stderr, "Legal\n");
		    break;
		case 6 :
		    fprintf(stderr, "Tabloid\n");
		    break;
		case 26 :
		    fprintf(stderr, "A4\n");
		    break;
		case 27 :
		    fprintf(stderr, "A3\n");
		    break;
		case 101 :
		    fprintf(stderr, "Custom\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_MEDIA_TYPE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "Plain\n");
		    break;
		case 1 :
		    fprintf(stderr, "Bond\n");
		    break;
		case 2 :
		    fprintf(stderr, "Premium\n");
		    break;
		case 3 :
		    fprintf(stderr, "Glossy\n");
		    break;
		case 4 :
		    fprintf(stderr, "Transparency\n");
		    break;
		case 5 :
		    fprintf(stderr, "Photo\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_MEDIA_SOURCE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "EJECT\n");
		    break;
		case 1 :
		    fprintf(stderr, "Tray 2\n");
		    break;
		case 2 :
		    fprintf(stderr, "Manual\n");
		    break;
		case 3 :
		    fprintf(stderr, "Envelope\n");
		    break;
		case 4 :
		    fprintf(stderr, "Tray 3\n");
		    break;
		case 5 :
		    fprintf(stderr, "Tray 4\n");
		    break;
		case 8 :
		    fprintf(stderr, "Tray 1\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_SHINGLING :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "None\n");
		    break;
		case 1 :
		    fprintf(stderr, "2 passes\n");
		    break;
		case 2 :
		    fprintf(stderr, "4 passes\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_RASTERGRAPHICS_QUALITY :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "(set by printer controls)\n");
		case 1 :
		    fprintf(stderr, "Draft\n");
		    break;
		case 2 :
		    fprintf(stderr, "High\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_DEPLETION :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 1 :
		    fprintf(stderr, "None\n");
		    break;
		case 2 :
		    fprintf(stderr, "25%%\n");
		    break;
		case 3 :
		    fprintf(stderr, "50%%\n");
		    break;
		case 5 :
		    fprintf(stderr, "50%% with gamma correction\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_PRINT_QUALITY :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case -1 :
		    fprintf(stderr, "Draft\n");
		    break;
		case 0 :
		    fprintf(stderr, "Normal\n");
		    break;
		case 1 :
		    fprintf(stderr, "Presentation\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_PERF_SKIP :
	    case PCL_TOP_MARGIN :
	    case PCL_RESOLUTION :
	    case PCL_LEFTRASTER_POS :
	    case PCL_TOPRASTER_POS :
	    case PCL_VERTICAL_CURSOR_POSITIONING_BY_DOTS :
	    case PCL_HORIZONTAL_CURSOR_POSITIONING_BY_DOTS :
	    case PCL_RELATIVE_VERTICAL_PIXEL_MOVEMENT :
	    case PCL_PALETTE_CONFIGURATION :
		fprintf(stderr, "%s: %d (ignored)", pcl_commands[command_index].description, numeric_arg);
		if (pcl_commands[command_index].has_data == 1) {
		    fprintf(stderr, " Data: ");
		    for (i=0; i < numeric_arg; i++) {
			fprintf(stderr, "%02x ", (unsigned char) data_buffer[i]);
		    }
		}
		fprintf(stderr, "\n");
		break;

	    case PCL_COLOURTYPE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		image_data.colour_type = -numeric_arg;
		switch (image_data.colour_type) {
		    case PCL_MONO :
			fprintf(stderr, "MONO\n");
			break;
		    case PCL_CMY :
			fprintf(stderr, "CMY (one cart)\n");
			image_data.black_depth = 0;	/* Black levels */
			image_data.cyan_depth = 2;	/* Cyan levels */
			image_data.magenta_depth = 2;	/* Magenta levels */
			image_data.yellow_depth = 2;	/* Yellow levels */
			break;
		    case PCL_CMYK :
			fprintf(stderr, "CMYK (two cart)\n");
			image_data.cyan_depth = 2;	/* Cyan levels */
			image_data.magenta_depth = 2;	/* Magenta levels */
			image_data.yellow_depth = 2;	/* Yellow levels */
			break;
		    default :
			fprintf(stderr, "Unknown (%d)\n", -numeric_arg);
			break;
		}
		break;

	    case PCL_COMPRESSIONTYPE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		image_data.compression_type = numeric_arg;
		switch (image_data.compression_type) {
		    case PCL_COMPRESSION_NONE :
			fprintf(stderr, "NONE\n");
			break;
		    case PCL_COMPRESSION_TIFF :
			fprintf(stderr, "TIFF\n");
			break;
		    default :
			fprintf(stderr, "Unknown (%d)\n", image_data.compression_type);
			break;
		}
		break;

	    case PCL_RASTER_WIDTH :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		image_data.image_width = numeric_arg;
		break;

	    case PCL_RASTER_HEIGHT :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		image_data.image_height = numeric_arg;
		break;

	    case PCL_CONFIGURE :
		fprintf(stderr, "%s (size=%d)\n", pcl_commands[command_index].description,
		    numeric_arg);

/*
 * the data that follows depends on the colour type (buffer[1]). The size
 * of the data should be 2 + (6 * number of planes).
 */

		fprintf(stderr, "\tFormat: %d, Output Planes: ", data_buffer[0]);
		image_data.colour_type = data_buffer[1]; 	/* # output planes */
		switch (image_data.colour_type) {
		    case PCL_MONO :
			fprintf(stderr, "MONO\n");

/* Size should be 8 */

			if (numeric_arg != 8)
			    fprintf(stderr, "ERROR: Expected 8 bytes of data, got %d\n", numeric_arg);

			fprintf(stderr, "\tBlack: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[2]<<8)+(unsigned char)data_buffer[3],
			    ((unsigned char) data_buffer[4]<<8)+(unsigned char) data_buffer[5], data_buffer[7]);
			image_data.black_depth = data_buffer[7];	/* Black levels */
			image_data.cyan_depth = 0;
			image_data.magenta_depth = 0;
			image_data.yellow_depth = 0;
			break;
		    case PCL_CMY :
			fprintf(stderr, "CMY (one cart)\n");

/* Size should be 20 */

			if (numeric_arg != 20)
			    fprintf(stderr, "ERROR: Expected 8 bytes of data, got %d\n", numeric_arg);

			fprintf(stderr, "\tCyan: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[2]<<8)+(unsigned char) data_buffer[3],
			    ((unsigned char) data_buffer[4]<<8)+(unsigned char) data_buffer[5], data_buffer[7]);
			fprintf(stderr, "\tMagenta: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[8]<<8)+(unsigned char) data_buffer[9],
			    ((unsigned char) data_buffer[10]<<8)+(unsigned char) data_buffer[11], data_buffer[13]);
			fprintf(stderr, "\tYellow: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[14]<<8)+(unsigned char) data_buffer[15],
			    ((unsigned char) data_buffer[16]<<8)+(unsigned char) data_buffer[17], data_buffer[19]);
			image_data.black_depth = 0;
			image_data.cyan_depth = data_buffer[7];		/* Cyan levels */
			image_data.magenta_depth = data_buffer[13];	/* Magenta levels */
			image_data.yellow_depth = data_buffer[19];	/* Yellow levels */
			break;
		    case PCL_CMYK :
			fprintf(stderr, "CMYK (two cart)\n");

/* Size should be 26 */

			if (numeric_arg != 26)
			    fprintf(stderr, "ERROR: Expected 8 bytes of data, got %d\n", numeric_arg);

			fprintf(stderr, "\tBlack: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[2]<<8)+(unsigned char) data_buffer[3],
			    ((unsigned char) data_buffer[4]<<8)+(unsigned char) data_buffer[5], data_buffer[7]);
			fprintf(stderr, "\tCyan: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[8]<<8)+(unsigned char) data_buffer[9],
			    ((unsigned char) data_buffer[10]<<8)+(unsigned char) data_buffer[11], data_buffer[13]);
			fprintf(stderr, "\tMagenta: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[14]<<8)+(unsigned char) data_buffer[15],
			    ((unsigned char) data_buffer[16]<<8)+(unsigned char) data_buffer[17], data_buffer[19]);
			fprintf(stderr, "\tYellow: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[20]<<8)+(unsigned char) data_buffer[21],
			    ((unsigned char) data_buffer[22]<<8)+(unsigned char) data_buffer[23], data_buffer[25]);
			image_data.black_depth = data_buffer[7];	/* Black levels */
			image_data.cyan_depth = data_buffer[13];	/* Cyan levels */
			image_data.magenta_depth = data_buffer[19];	/* Magenta levels */
			image_data.yellow_depth = data_buffer[25];	/* Yellow levels */
			break;
		    default :
			fprintf(stderr, "Unknown (%d)\n", data_buffer[1]);
			break;
		}

		break;

	    case PCL_DATA :
	    case PCL_DATA_LAST :
#ifdef DEBUG
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);
		fprintf(stderr, "Data Length: %d\n", numeric_arg);
#endif

/*
 * Make sure that we have enough data to process this command!
 */

		if (expected_data_rows_per_row == -1)
		    fprintf(stderr, "ERROR: raster data without start raster!\n");

/* 
 * The last flag indicates that this is the end of the planes for a row
 * so we check it against the number of planes we have seen and are
 * expecting.
 */

		if (command == PCL_DATA_LAST) {
		    if (current_data_row != (expected_data_rows_per_row - 1))
			fprintf(stderr, "ERROR: 'Last Plane' set on plane %d of %d!\n",
			    current_data_row, expected_data_rows_per_row);
		}
		else {
		    if (current_data_row == (expected_data_rows_per_row - 1))
			fprintf(stderr, "ERROR: Expected 'last plane', but not set!\n");
		}

/*
 * Accumulate the data rows for each output row,then write the image.
 */

		if (image_data.compression_type == PCL_COMPRESSION_NONE) {
		    memcpy(received_rows[current_data_row], &data_buffer, (size_t) numeric_arg);
		    output_data.active_length = numeric_arg;
		}
		else
		    output_data.active_length = decode_tiff(data_buffer, numeric_arg, received_rows[current_data_row], MAX_DATA);

		if (command == PCL_DATA_LAST) {
		    if (image_data.colour_type == PCL_MONO)
			write_grey(&output_data, &image_data);
		    else
			write_colour(&output_data, &image_data);
		    current_data_row = 0;
		    image_row_counter++;
		}
		else
		    current_data_row++;

		break;

	    case PCL_PJL_COMMAND : {
		    int c;
		    fprintf(stderr, "%s: ", pcl_commands[command_index].description);

/*
 * This is a special command, actually it is a PJL instruction. Read up
 * to the next NL and output it.
 */

		    c = 0;
		    while (c != '\n') {
			fill_buffer();
			if (eof == 1) {
			    fprintf(stderr, "ERROR: EOF looking for EOL!\n");
			    break;
			}
			c = read_buffer[read_pointer];
			fprintf(stderr, "%c", c);
			read_pointer++;
		    }
		}
		break;

	    case PCL_PAGE_ORIENTATION :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "Portrait");
		    break;
		case 1 :
		    fprintf(stderr, "Landscape");
		    break;
		case 2 :
		    fprintf(stderr, "Reverse Portrait");
		    break;
		case 3 :
		    fprintf(stderr, "Reverse Landscape");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)", numeric_arg);
		    break;
		}
		fprintf(stderr, " (ignored)\n");
		break;

	    case PCL_UNK1 :
	    case PCL_UNK2 :
	    case PCL_UNK3 :
		fprintf(stderr, "ERROR: Unknown command: %s%d%c", initial_command,
		    numeric_arg, final_command);
		if (pcl_commands[command_index].has_data == 1) {
		    fprintf(stderr, " Data: ");
		    for (i=0; i < numeric_arg; i++) {
			fprintf(stderr, "%02x ", (unsigned char) data_buffer[i]);
		    }
		}
		fprintf(stderr, "\n");
		break;

	    default :
		fprintf(stderr, "ERROR: No handler for %s!\n", pcl_commands[command_index].description);
		break;
	    }
	}
    }
}

/*
 * Revision History:
 *
 *   $Log$
 *   Revision 1.7  2000/03/21 19:10:37  davehill
 *   Use unsigned when calculating resolutions. Updated some commands.
 *
 *   Revision 1.6  2000/03/20 21:03:30  davehill
 *   Added "Bond" and "Photo" paper types to pcl-unprint and print-pcl.
 *   Corrected Depletion output for old Deskjets in print-pcl.
 *
 *   Revision 1.5  2000/02/28 18:39:24  davehill
 *   Fixed decoding of "configure data". Added "Custom" to paper sizes.
 *   Started changes for multiple levels.
 *
 *   Revision 1.4  2000/02/23 20:33:32  davehill
 *   Added more commands to the commans set.
 *   Now handles repeated commands that share the same prefix.
 *
 *   Revision 1.3  2000/02/21 15:12:57  rlk
 *   Minor release prep
 *
 *   Revision 1.2  2000/02/20 20:52:31  davehill
 *   Now does TIFF compressed files and 2 level colour in CMY
 *   or CMYK
 *
 *   Revision 1.1  2000/02/17 01:02:13  rlk
 *   Rename various programs
 *
 *   Revision 1.1  2000/02/15 22:06:38  davehill
 *   Initial version. Only works for mono 2 level uncompressed output
 *   (i.e. Deskjet 500)
 *
 *
 * End of "$Id$"
 */

