/*
 *   Canon SELPHY ES/CP series print assister -- Common Code
 *
 *   (c) 2007-2012 Solomon Peachy <pizza@shaftnet.org>
 *
 *   The latest version of this program can be found at
 *  
 *   http://git.shaftnet.org/git/gitweb.cgi?p=selphy_print.git
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
 *          [http://www.gnu.org/licenses/gpl-2.0.html]
 *
 */

#define VERSION "0.43"

#define DEBUG( ... ) fprintf(stderr, "DEBUG: " __VA_ARGS__ )
#define ERROR( ... ) fprintf(stderr, "ERROR: " __VA_ARGS__ )

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define le32_to_cpu(__x) __x
#else
#define le32_to_cpu(x)							\
	({								\
		uint32_t __x = (x);					\
		((uint32_t)(						\
			(((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
			(((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
			(((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
			(((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
	})
#endif

#define READBACK_LEN 12

struct printer_data {
	int  type;  /* P_??? */
	char *model; /* eg "SELPHY ES1" */
	int  init_length;
	int  foot_length;
	int16_t init_readback[READBACK_LEN];
	int16_t ready_y_readback[READBACK_LEN];
	int16_t ready_m_readback[READBACK_LEN];
	int16_t ready_c_readback[READBACK_LEN];
	int16_t done_c_readback[READBACK_LEN];
	int16_t paper_codes[256];
	int16_t pgcode_offset;  /* Offset into printjob for paper type */
	int16_t paper_code_offset; /* Offset in readback for paper type */
	int16_t error_offset;
};

/* printer types */
enum {
	P_ES1 = 0,
	P_ES2_20,
	P_ES3_30,
	P_ES40_CP790,
	P_CP_XXX,
	P_CP10,
	P_END
};

struct printer_data printers[P_END] = {
	{ .type = P_ES1,
	  .model = "SELPHY ES1",
	  .init_length = 12,
	  .foot_length = 0,
	  .init_readback = { 0x02, 0x00, 0x00, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x04, 0x00, 0x01, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x04, 0x00, 0x03, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x04, 0x00, 0x07, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x04, 0x00, 0x00, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  // .paper_codes
	  .pgcode_offset = 3,
	  .paper_code_offset = 6,
	  .error_offset = 1,
	},
	{ .type = P_ES2_20,
	  .model = "SELPHY ES2/ES20",
	  .init_length = 16,
	  .foot_length = 0,
	  .init_readback = { 0x02, 0x00, 0x00, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x03, 0x00, 0x01, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x06, 0x00, 0x03, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x09, 0x00, 0x07, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x09, 0x00, 0x00, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  // .paper_codes
	  .pgcode_offset = 2,
	  .paper_code_offset = 4,
	  .error_offset = 1,  // XXX insufficient
	},
	{ .type = P_ES3_30,
	  .model = "SELPHY ES3/ES30",
	  .init_length = 16,
	  .foot_length = 12,
	  .init_readback = { 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x01, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x03, 0xff, 0x02, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x05, 0xff, 0x03, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x00, 0xff, 0x10, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  // .paper_codes
	  .pgcode_offset = 2,
	  .paper_code_offset = -1,
	  .error_offset = 8, // or 10
	},
	{ .type = P_ES40_CP790,
	  .model = "SELPHY ES40/CP790",
	  .init_length = 16,
	  .foot_length = 12,
	  .init_readback = { 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_y_readback = { 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_m_readback = { 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_c_readback = { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .done_c_readback = { 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  // .paper_codes
	  .pgcode_offset = 2,
	  .paper_code_offset = 11,
	  .error_offset = 3,
	},
	{ .type = P_CP_XXX,
	  .model = "SELPHY CP Series (!CP-10/CP790)",
	  .init_length = 12,
	  .foot_length = 0,  /* CP900 has four-byte NULL footer that can be safely ignored */
	  .init_readback = { 0x01, 0x00, 0x00, 0x00, -1, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_y_readback = { 0x02, 0x00, 0x00, 0x00, 0x70, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_m_readback = { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_c_readback = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .done_c_readback = { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  // .paper_codes
	  .pgcode_offset = 3,
	  .paper_code_offset = 6,
	  .error_offset = 2,
	},
	{ .type = P_CP10,
	  .model = "SELPHY CP-10",
	  .init_length = 12,
	  .foot_length = 0,
	  .init_readback = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  // .paper_codes
	  .pgcode_offset = -1,
	  .paper_code_offset = -1,
	  .error_offset = 2,
	},
};

#define MAX_HEADER 28
#define BUF_LEN 4096

static const int es40_cp790_plane_lengths[4] = { 2227456, 1601600, 698880, 2976512 };

static void setup_paper_codes(void)
{
	/* Default all to IGNORE */
	int i, j;
	for (i = 0; i < P_END ; i++)
		for (j = 0 ; j < 256 ; j++) 
			printers[i].paper_codes[j] = -1;
	
	/* SELPHY ES1 paper codes */
	printers[P_ES1].paper_codes[0x11] = 0x01;
	printers[P_ES1].paper_codes[0x12] = 0x02; // ? guess
	printers[P_ES1].paper_codes[0x13] = 0x03;
	
	/* SELPHY ES2/20 paper codes */
	printers[P_ES2_20].paper_codes[0x01] = 0x01;
	printers[P_ES2_20].paper_codes[0x02] = 0x02; // ? guess
	printers[P_ES2_20].paper_codes[0x03] = 0x03;
	
	/* SELPHY ES3/30 paper codes -- N/A, printer does not report paper type */	
	/* SELPHY ES40/CP790 paper codes -- ? guess */
	printers[P_ES40_CP790].paper_codes[0x00] = 0x11;
	printers[P_ES40_CP790].paper_codes[0x01] = 0x22;
	printers[P_ES40_CP790].paper_codes[0x02] = 0x33;
	printers[P_ES40_CP790].paper_codes[0x03] = 0x44;

	/* SELPHY CP-series (except CP790) paper codes */
	printers[P_CP_XXX].paper_codes[0x01] = 0x11;
	printers[P_CP_XXX].paper_codes[0x02] = 0x22;
	printers[P_CP_XXX].paper_codes[0x03] = 0x33;
	printers[P_CP_XXX].paper_codes[0x04] = 0x44;

	/* SELPHY CP-10 paper codes -- N/A, only one type */
}

#define INCORRECT_PAPER -999

/* Program states */
enum {
	S_IDLE = 0,
	S_PRINTER_READY,
	S_PRINTER_INIT_SENT,
	S_PRINTER_READY_Y,
	S_PRINTER_Y_SENT,
	S_PRINTER_READY_M,
	S_PRINTER_M_SENT,
	S_PRINTER_READY_C,
	S_PRINTER_C_SENT,
	S_PRINTER_DONE,
	S_FINISHED,
};

