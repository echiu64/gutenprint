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
#include<ctype.h>

static char *id="@(#) $Id$";

FILE *read_fd,*write_fd;
char read_buffer[1024];
char data_buffer[1024];
char initial_command[10];
char final_command;
int numeric_arg;

int read_pointer;
int read_size;
int eof;

#define PCL_MONO 1
#define PCL_CMY 2
#define PCL_CMYK 3

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
#define PCL_QUALITY 8
#define PCL_DEPLETION 9
#define PCL_CONFIGURE 10
#define PCL_RESOLUTION 11
#define PCL_COLORTYPE 12
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
	{ "*o", 'Q', 0, PCL_SHINGLING, "Shingling" },
	{ "*r", 'Q', 0, PCL_QUALITY, "Quality" },
	{ "*o", 'D', 0, PCL_DEPLETION, "Depletion" },
	{ "*g", 'W', 1, PCL_CONFIGURE, "Extended Configure" },
	{ "*t", 'R', 0, PCL_RESOLUTION, "Resolution" },
	{ "*r", 'U', 0, PCL_COLORTYPE, "Color Type" },
	{ "*b", 'M', 0, PCL_COMPRESSIONTYPE, "Compression Type" },
	{ "&a", 'H', 0, PCL_LEFTRASTER_POS, "Left Raster Position" },
	{ "&a", 'V', 0, PCL_TOPRASTER_POS, "Top Raster Position" },
	{ "*r", 'S', 0, PCL_RASTER_WIDTH, "Raster Width" },
	{ "*r", 'T', 0, PCL_RASTER_HEIGHT, "Raster Height" },
	{ "*r", 'A', 0, PCL_START_RASTER, "Start Raster Graphics" },
	{ "*rB", '\0', 0, PCL_END_RASTER, "End Raster Graphics"},
	{ "*rbC", '\0', 0, PCL_END_RASTER_NEW, "End Raster Graphics" },
	{ "*b", 'V', 1, PCL_DATA, "Data, intermediate" },
	{ "*b", 'W', 1, PCL_DATA_LAST, "Data, last" },
   };

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

void pcl_read_command() {

    int command_index;
    char c;
    int minus;

/* 
   The format of a PCL command is: ESC <x> <y> [n] <z>
   where x, y and z are characters, and [n] is an optional number.
   Some commands are followed by data, in this case, [n] is the
   number of bytes to read.
   An exception is "ESC E" (reset).
*/

    command_index=0;
    initial_command[command_index] = '\0';
    numeric_arg=0;
    minus = 0;
    final_command = '\0';

    fill_buffer();
    if (eof == 1)
        return;

/* First character must be ESC, otherwise we have gone wrong! */

    c = read_buffer[read_pointer];
    if(c != (char) 0x1b) {
        fprintf(stderr, "No ESC found (out of sync?)\n");
        exit(2);
    }
    read_pointer++;
    fill_buffer();
    if (eof == 1) {

#ifdef DEBUG
        fprintf(stderr, "EOF after ESC!\n");
#endif

        return;
    }

/* Get first command letter */

    c = read_buffer[read_pointer];
    initial_command[command_index] = c;

#ifdef DEBUG
    fprintf(stderr, "Got %c\n", c);
#endif

    read_pointer++;

    command_index++;

/* Now keep going until we find a numeric, or another ESC, or EOF */

    while (1) {
        fill_buffer();
        if (eof == 1) {

#ifdef DEBUG
            fprintf(stderr, "EOF in middle of command!\n");
#endif
	    eof = 0;		/* Ignore it */
	    initial_command[command_index] = '\0';
            return;
        }
        c = read_buffer[read_pointer];
	if (c == (char) 0x1b) {

#ifdef DEBUG
	    fprintf(stderr, "Got another ESC!\n");
#endif

	    command_index++;
	    initial_command[command_index] = '\0';
	    return;
	}

#ifdef DEBUG
        fprintf(stderr, "Got %c\n", c);
#endif

        read_pointer++;
        if ((isdigit(c)) || (c == '-'))
            break;
        initial_command[command_index] = c;
        command_index++;
    }

    initial_command[command_index] = '\0';

    if (c == '-')
	minus = 1;
    else
	numeric_arg = (int) (c - '0');

    while (1) {
        fill_buffer();
        if (eof == 1) {
            fprintf(stderr, "EOF in middle of command!\n");
            return;
        }
        c = read_buffer[read_pointer];

#ifdef DEBUG
        fprintf(stderr, "Got %c\n", c);
#endif

        read_pointer++;
        command_index++;
        if (! isdigit(c)) {
            final_command = c;
	    if (minus == 1)
		numeric_arg = -numeric_arg;
            return;
        }
        numeric_arg = (10 * numeric_arg) + (int) (c - '0');
    }
}

int main(int argc,char *argv[]){

    int command_index;
    int command;
    int i,j;
    char tb[8];
    int wholebytes, crumbs;

/* Things we need to know before we can output the image */
/* Some things default if not given */

    int color_type = PCL_MONO;		/* Mono, 3/4 colour */
    int black_depth = 2;		/* 2 level, 4 level */
    int cyan_depth = 2;			/* 2 level, 4 level */
    int magenta_depth = 2;		/* 2 level, 4 level */
    int yellow_depth = 2;		/* 2 level, 4 level */
    int image_width = -1;
    int image_height = -1;
    int compression_type = -1;		/* Uncompressed or TIFF */

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
        fprintf(stderr, "Error Opening input file.\n");
        exit (1);
    }

    if (write_fd == (FILE *)NULL) {
        fprintf(stderr, "Error Opening output file.\n");
        exit (1);
    }

    read_pointer=-1;
    eof=0;

    while (1) {
        pcl_read_command();
        if (eof == 1) {
#ifdef DEBUG
            fprintf(stderr, "EOF while reading command.\n");
#endif
            (void) fclose(read_fd);
            (void) fclose(write_fd);
            exit(0);
        }

#ifdef DEBUG
        fprintf(stderr, "initial_command: %s, numeric_arg: %d, final_command: %c\n",
            initial_command, numeric_arg, final_command);
#endif

	command_index = pcl_find_command();
        if (command_index == -1) {
            fprintf(stderr, "Unknown command: %s%d%c\n", initial_command,
                numeric_arg, final_command);
	}
	else {
	    command = pcl_commands[command_index].command;
	    if (pcl_commands[command_index].has_data == 1) {

/* Read the data into data_buffer */

#ifdef DEBUG
		fprintf(stderr, "Data: ");
#endif

		for (i=0; i < numeric_arg; i++) {
		    fill_buffer();
		    if (eof == 1) {
			fprintf(stderr, "Unexpected EOF whilst reading data\n");
			exit(3);
		    }
		    data_buffer[i] = read_buffer[read_pointer];

#ifdef DEBUG
		    fprintf(stderr, "%04x ", data_buffer[i]);
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
		color_type = PCL_MONO;
		black_depth = 2;
		cyan_depth = 2;
		magenta_depth = 2;
		yellow_depth = 2;
		image_width = -1;
		image_height = -1;
		compression_type = -1;
		break;

	    case PCL_START_RASTER :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);

/* Make sure we have all the stuff needed to work out what we are going
   to write out. */

		if (image_width == -1)
		    fprintf(stderr, "Error: Image width not set!\n");
		if (image_height == -1)
		    fprintf(stderr, "Error: Image height not set!\n");

		if (color_type != PCL_MONO) {
		    fprintf(stderr, "Sorry, only MONO handled.\n");
		    exit (4);
		}
		if (black_depth != 2) {
		    fprintf(stderr, "Sorry, only 2 level dithers handled.\n");
		    exit (4);
		}
		if (compression_type != PCL_COMPRESSION_NONE) {
		    fprintf(stderr, "Sorry, only 'no compression' handled.\n");
		    exit(4);
		}
		if (color_type == PCL_MONO)
		    (void) fputs("P5\n", write_fd);	/* Raw, Grey */
		else
		    (void) fputs("P6\n", write_fd);	/* Raw, RGB */

		(void) fputs("# Written by pclunprint.\n", write_fd);

		fprintf(write_fd, "%d %d\n255\n", image_width, image_height);
		
		break;

	    case PCL_END_RASTER :
	    case PCL_END_RASTER_NEW :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);
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
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		}
		break;

	    case PCL_MEDIA_TYPE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "Plain\n");
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
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		}
		break;

	    case PCL_MEDIA_SOURCE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "EJECT\n");
		    break;
		case 2 :
		    fprintf(stderr, "Manual\n");
		    break;
		case 8 :
		    fprintf(stderr, "Tray 1\n");
		    break;
		case 1 :
		    fprintf(stderr, "Tray 2\n");
		    break;
		case 4 :
		    fprintf(stderr, "Tray 3\n");
		    break;
		case 5 :
		    fprintf(stderr, "Tray 4\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		}
		break;

	    case PCL_PERF_SKIP :
	    case PCL_TOP_MARGIN :
	    case PCL_SHINGLING :
	    case PCL_QUALITY :
	    case PCL_DEPLETION :
	    case PCL_RESOLUTION :
	    case PCL_LEFTRASTER_POS :
	    case PCL_TOPRASTER_POS :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		break;

	    case PCL_COLORTYPE :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		color_type = numeric_arg;
		break;

	    case PCL_COMPRESSIONTYPE :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		compression_type = numeric_arg;
		break;

	    case PCL_RASTER_WIDTH :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		image_width = numeric_arg;
		break;

	    case PCL_RASTER_HEIGHT :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		image_height = numeric_arg;
		break;

	    case PCL_CONFIGURE :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);
		fprintf(stderr, "Format: %d, Output Planes: %d\n", data_buffer[0], data_buffer[1]);
		fprintf(stderr, "Black: X dpi: %d, Y dpi: %d, Levels: %d\n", (data_buffer[2]<<8)+data_buffer[3],
		    (data_buffer[4]<<8)+data_buffer[5], data_buffer[7]);
		fprintf(stderr, "Cyan: X dpi: %d, Y dpi: %d, Levels: %d\n", (data_buffer[8]<<8)+data_buffer[9],
		    (data_buffer[10]<<8)+data_buffer[11], data_buffer[13]);
		fprintf(stderr, "Magenta: X dpi: %d, Y dpi: %d, Levels: %d\n", (data_buffer[14]<<8)+data_buffer[15],
		    (data_buffer[16]<<8)+data_buffer[17], data_buffer[19]);
		fprintf(stderr, "Cyan: X dpi: %d, Y dpi: %d, Levels: %d\n", (data_buffer[20]<<8)+data_buffer[21],
		    (data_buffer[22]<<8)+data_buffer[23], data_buffer[25]);

		color_type = data_buffer[1]; 	/* # output planes */
		black_depth = data_buffer[7];	/* Black levels */
		cyan_depth = data_buffer[13];	/* Cyan levels */
		magenta_depth = data_buffer[19];/* Magenta levels */
		yellow_depth = data_buffer[25];	/* Yellow levels */
		break;

	    case PCL_DATA :
	    case PCL_DATA_LAST :
#ifdef DEBUG
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);
#endif

/* The last flag indicates that this is the end of the planes for a row */
/* for now, just write out the data, since we only handle mono, 2 level */
/* To convert into PNM format, we have to write out 1 byte per bit of
   the input data */
/* The number of bits of data is given by the image width, there may be
   some crumbs in the last byte of data */

		wholebytes = image_width / 8;
		crumbs = image_width - (wholebytes * 8);

#ifdef DEBUG
		fprintf(stderr, "Data Length: %d, wholebytes: %d, crumbs: %d\n",
		    numeric_arg, wholebytes, crumbs);
#endif

		if (compression_type == PCL_COMPRESSION_NONE) {
		    for (i=0; i < wholebytes; i++) {
			for (j=0; j < 8; j++) {
			    tb[j] = 255- (((data_buffer[i] >> (7-j)) & 1)*255);
			}
			(void) fwrite(&tb[0], sizeof(char), 8, write_fd);
		    }
		    for (j=0; j < crumbs; j++) {
			tb[j] = 255- (((data_buffer[wholebytes] >> (7-j)) & 1)*255);
		    }
		    (void) fwrite(&tb[0], sizeof(char), (size_t) crumbs, write_fd);
		}
		else if (compression_type == PCL_COMPRESSION_TIFF) {
		}
		break;

	    default :
		fprintf(stderr, "No handler for %s!\n", pcl_commands[command_index].description);
		break;
	    }
	}
    }
}

/*
 * Revision History:
 *
 *   $Log$
 *   Revision 1.1  2000/02/15 22:06:38  davehill
 *   Initial version. Only works for mono 2 level uncompressed output
 *   (i.e. Deskjet 500)
 *
 *
 * End of "$Id$"
 */

