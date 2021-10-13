/*
 *   Mitsubishi Photo Printer Comon Code
 *
 *   (c) 2013-2021 Solomon Peachy <pizza@shaftnet.org>
 *
 *   The latest version of this program can be found at:
 *
 *     https://git.shaftnet.org/cgit/selphy_print.git
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 *   SPDX-License-Identifier: GPL-2.0+
 *
 */

#include "backend_common.h"
#include "backend_mitsu.h"

int mitsu_loadlib(struct mitsu_lib *lib, int type)
{
	memset(lib, 0, sizeof(*lib));

#if defined(WITH_DYNAMIC)
	DL_INIT();

	DEBUG("Attempting to load image processing library\n");
	lib->dl_handle = DL_OPEN(LIB_NAME_RE);
	if (!lib->dl_handle)
		WARNING("Image processing library not found, using internal fallback code\n");
	if (lib->dl_handle) {
		lib->GetAPIVersion = DL_SYM(lib->dl_handle, "lib70x_getapiversion");
		if (!lib->GetAPIVersion) {
			ERROR("Problem resolving API Version symbol in imaging processing library, too old or not installed?\n");
			DL_CLOSE(lib->dl_handle);
			lib->dl_handle = NULL;
			return CUPS_BACKEND_FAILED;
		}
		if (lib->GetAPIVersion() != REQUIRED_LIB_APIVERSION) {
			ERROR("Image processing library API version mismatch! (%d vs %d)\n", lib->GetAPIVersion(), REQUIRED_LIB_APIVERSION);
			DL_CLOSE(lib->dl_handle);
			lib->dl_handle = NULL;
			return CUPS_BACKEND_FAILED;
		}
		lib->DumpAnnounce = DL_SYM(lib->dl_handle, "dump_announce");

		lib->Get3DColorTable = DL_SYM(lib->dl_handle, "CColorConv3D_Get3DColorTable");
		lib->Load3DColorTable = DL_SYM(lib->dl_handle, "CColorConv3D_Load3DColorTable");
		lib->Destroy3DColorTable = DL_SYM(lib->dl_handle, "CColorConv3D_Destroy3DColorTable");
		lib->DoColorConv = DL_SYM(lib->dl_handle, "CColorConv3D_DoColorConv");
		lib->DoColorConvPlane = DL_SYM(lib->dl_handle, "CColorConv3D_DoColorConvPlane");
		lib->GetCPCData = DL_SYM(lib->dl_handle, "get_CPCData");
		lib->DestroyCPCData = DL_SYM(lib->dl_handle, "destroy_CPCData");
		lib->DoImageEffect60 = DL_SYM(lib->dl_handle, "do_image_effect60");
		lib->DoImageEffect70 = DL_SYM(lib->dl_handle, "do_image_effect70");
		lib->DoImageEffect80 = DL_SYM(lib->dl_handle, "do_image_effect80");
		lib->SendImageData = DL_SYM(lib->dl_handle, "send_image_data");
		lib->CP98xx_DoConvert = DL_SYM(lib->dl_handle, "CP98xx_DoConvert");
		lib->CP98xx_GetData = DL_SYM(lib->dl_handle, "CP98xx_GetData");
		lib->CP98xx_DestroyData = DL_SYM(lib->dl_handle, "CP98xx_DestroyData");
		lib->M1_GetCPCData = DL_SYM(lib->dl_handle, "M1_GetCPCData");
		lib->M1_DestroyCPCData = DL_SYM(lib->dl_handle, "M1_DestroyCPCData");
		lib->M1_Gamma8to14 = DL_SYM(lib->dl_handle, "M1_Gamma8to14");
		lib->M1_CLocalEnhancer = DL_SYM(lib->dl_handle, "M1_CLocalEnhancer");
		lib->M1_CalcRGBRate = DL_SYM(lib->dl_handle, "M1_CalcRGBRate");
		lib->M1_CalcOpRateMatte = DL_SYM(lib->dl_handle, "M1_CalcOpRateMatte");
		lib->M1_CalcOpRateGloss = DL_SYM(lib->dl_handle, "M1_CalcOpRateGloss");
		lib->CPD30_GetData = DL_SYM(lib->dl_handle, "CPD30_GetData");
		lib->CPD30_DestroyData = DL_SYM(lib->dl_handle, "CPD30_DestroyData");
		lib->CPD30_DoConvert = DL_SYM(lib->dl_handle, "CPD30_DoConvert");

		if (!lib->Get3DColorTable || !lib->Load3DColorTable ||
		    !lib->CP98xx_DoConvert || !lib->CP98xx_GetData ||
		    !lib->CP98xx_DestroyData ||
		    !lib->M1_GetCPCData || !lib->M1_DestroyCPCData ||
		    !lib->M1_Gamma8to14 || !lib->M1_CLocalEnhancer ||
		    !lib->M1_CalcOpRateMatte || !lib->M1_CalcOpRateGloss ||
		    !lib->M1_CalcRGBRate ||
		    !lib->CPD30_GetData || !lib->CPD30_DestroyData ||
		    !lib->CPD30_DoConvert ||
		    !lib->Destroy3DColorTable || !lib->DoColorConv ||
		    !lib->DoColorConvPlane ||
		    !lib->GetCPCData || !lib->DestroyCPCData ||
		    !lib->DoImageEffect60 || !lib->DoImageEffect70 ||
		    !lib->DoImageEffect80 || !lib->SendImageData) {
			ERROR("Problem resolving symbols in imaging processing library\n");
			DL_CLOSE(lib->dl_handle);
			lib->dl_handle = NULL;
			return CUPS_BACKEND_FAILED;
		} else {
			DEBUG("Image processing library successfully loaded\n");
			if (!stats_only && lib->DumpAnnounce)
				lib->DumpAnnounce(logger);
		}
	}

	switch (type) {
	case P_MITSU_D80:
		lib->DoImageEffect = lib->DoImageEffect80;
		break;
	case P_MITSU_K60:
	case P_KODAK_305:
		lib->DoImageEffect = lib->DoImageEffect60;
		break;
	case P_MITSU_D70X:
	case P_FUJI_ASK300:
		lib->DoImageEffect = lib->DoImageEffect70;
		break;
	case P_MITSU_9800:
	case P_MITSU_9800S:
	case P_MITSU_9810:
	default:
		lib->DoImageEffect = NULL;
	}

	return CUPS_BACKEND_OK;
#else
	ERROR("Need dynamic library support for library loading!\n");
	return CUPS_BACKEND_FAILED;
#endif
}

int mitsu_destroylib(struct mitsu_lib *lib)
{
#if defined(WITH_DYNAMIC)
	if (lib->dl_handle) {
		if (lib->cpcdata)
			lib->DestroyCPCData(lib->cpcdata);
		if (lib->ecpcdata)
			lib->DestroyCPCData(lib->ecpcdata);
		if (lib->lut)
			lib->Destroy3DColorTable(lib->lut);
		DL_CLOSE(lib->dl_handle);
	}

	memset(lib, 0, sizeof(*lib));
	DL_EXIT();

#endif
	return CUPS_BACKEND_OK;
}

int mitsu_apply3dlut_packed(struct mitsu_lib *lib, const char *lutfname, uint8_t *databuf,
			    uint16_t cols, uint16_t rows, uint16_t stride,
			    int rgb_bgr)
{
#if defined(WITH_DYNAMIC)
	char full[2048];
	int i;

	if (!lutfname)
		return CUPS_BACKEND_OK;
	if (!lib->dl_handle)
		return CUPS_BACKEND_OK;

	snprintf(full, sizeof(full), "%s/%s", corrtable_path, lutfname);

	if (!lib->lut) {
		uint8_t *buf = malloc(LUT_LEN);
		if (!buf) {
			ERROR("Memory allocation failure!\n");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		if ((i = dyesub_read_file(full, buf, LUT_LEN, NULL)))
			return i;
		lib->lut = lib->Load3DColorTable(buf);
		free(buf);
		if (!lib->lut) {
			ERROR("Unable to parse LUT file '%s'!\n", full);
			return CUPS_BACKEND_CANCEL;
		}
	}

	if (lib->lut) {
		DEBUG("Running print data through 3D LUT\n");
		lib->DoColorConv(lib->lut, databuf, cols, rows, stride, rgb_bgr);
	}
#endif
	return CUPS_BACKEND_OK;
}

int mitsu_apply3dlut_plane(struct mitsu_lib *lib, const char *lutfname,
			   uint8_t *data_r, uint8_t *data_g, uint8_t *data_b,
			   uint16_t cols, uint16_t rows)
{
#if defined(WITH_DYNAMIC)
	char full[2048];
	int i;

	if (!lutfname)
		return CUPS_BACKEND_OK;
	if (!lib->dl_handle)
		return CUPS_BACKEND_OK;

	snprintf(full, sizeof(full), "%s/%s", corrtable_path, lutfname);

	if (!lib->lut) {
		uint8_t *buf = malloc(LUT_LEN);
		if (!buf) {
			ERROR("Memory allocation failure!\n");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		if ((i = dyesub_read_file(full, buf, LUT_LEN, NULL)))
			return i;
		lib->lut = lib->Load3DColorTable(buf);
		free(buf);
		if (!lib->lut) {
			ERROR("Unable to parse LUT file '%s'!\n", full);
			return CUPS_BACKEND_CANCEL;
		}
	}

	if (lib->lut) {
		DEBUG("Running print data through 3D LUT\n");
		lib->DoColorConvPlane(lib->lut, data_r, data_g, data_b, cols * rows);
	}
#endif
	return CUPS_BACKEND_OK;
}

int mitsu_readlamdata(const char *fname, uint16_t lamstride,
		      uint8_t *databuf, uint32_t *datalen,
		      uint16_t rows, uint16_t cols, uint8_t bpp)
{
	int i, j, fd;
	char full[2048];

	snprintf(full, sizeof(full), "%s/%s", corrtable_path, fname);

	DEBUG("Reading %d bytes of matte data from disk (%d/%d)\n", cols * rows * bpp, cols, lamstride);
	fd = open(full, O_RDONLY);
	if (fd < 0) {
		ERROR("Unable to open matte lamination data file '%s'\n", full);
		return CUPS_BACKEND_CANCEL;
	}

	/* Read in the matte data plane */
	for (j = 0 ; j < rows ; j++) {
		int remain = lamstride * bpp;

		/* Read one row of lamination data at a time */
		while (remain) {
			i = read(fd, databuf + *datalen, remain);
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			if (i == 0) {
				/* We hit EOF, restart from beginning */
				lseek(fd, 0, SEEK_SET);
				continue;
			}
			*datalen += i;
			remain -= i;
		}
		/* Back off the buffer so we "wrap" on the print row. */
		*datalen -= ((lamstride - cols) * bpp);
	}

	return CUPS_BACKEND_OK;
}

const char *mitsu_temperatures(uint8_t temp)
{
	switch(temp) {
	case TEMPERATURE_NORMAL:
		return "Normal";
	case TEMPERATURE_PREHEAT:
		return "Warming Up";
	case TEMPERATURE_COOLING:
		return "Cooling Down";
	default:
		break;
	}
	return "Unknown Temperature Status";
}

const char *mitsu_media_types(int printer, uint8_t brand, uint8_t type)
{
	if (brand == 0xff) {  /* Mitsubishi */
		if (printer == P_MITSU_M1) {
			if (type == 0x02)
				return "CK-M46S (4x6)";
			else if (type == 0x04)
				return "CK-M57S (5x7)";
			else if (type == 0x0f)
				return "CK-M68S (6x8)";
		} else if (printer == P_MITSU_D80) {
			if (type == 0x0f)
				return "CK-D868 (6x8)";
		} else if (printer == P_MITSU_D90) {
			if (type == 0x0f)
				return "CK-D768/CK-D868 (6x8)";
		}

		/* Mitsu D70, and D90 fallthrough */
		if (type == 0x01)
			return "CK-D735 (3.5x5)";
		else if (type == 0x02)
			return "CK-D746 (4x6)";
		else if (type == 0x04)
			return "CK-D757 (5x7)";
		else if (type == 0x05)
			return "CK-D769 (6x9)";
		else if (type == 0x0f)
			return "CK-D768 (6x8)";
	} else if (brand == 0x61) { /* Mitsubishi (K60 series) */
		if (type == 0x84)
			return "CK-K57R (5x7)";
		else if (type == 0x8f)
			return "CK-K76R (6x8)";
	} else if (brand == 0x6c) { /* Kodak */
		if (type == 0x84)
			return "Kodak 5R (5x7)";
		else if (type == 0x8f)
			return "Kodak 6R (6x8)";
	} else if (brand == 0x7a) { /* Fujifilm*/
		if (type == 0x01)
			return "RL-CF900 (3.5x5)";
		else if (type == 0x02)
			return "RK-CF800/4R (4x6)";
		else if (type == 0x04)
			return "R2L-CF460/5R (5x7)";
		else if (type == 0x0f)
			return "R68-CF400/6R (6x8)";
	} else if (brand == 0xd1) { /* Mitsubishi (D70/D80 -S series) */
		if (type == 0x02)
			return "CK-D715 (4x6)";
		else if (type == 0x04)
			return "CK-D718 (5x7)";
		else if (type == 0x05)
			return "CK-D723 (6x9)";
		else if (type == 0x0f)
			return "CK-D720 (6x8)";
	}

	return "Unknown";

// Also CK-D746-U for D70-U model
//      CK-D820 (6x8) for D80-S model
// D90 can use _all_ of these types except for the -U!

	// CK-M15S  (6x4 for M15)
	// CK-M18S  (5x7 for M15)
	// CK-M20S  (6x8 for M15)
}
