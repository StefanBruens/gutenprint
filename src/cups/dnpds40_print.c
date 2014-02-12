/*
 *   DNP DS40/DS80 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2014 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Development of this backend was sponsored by:
 *
 *       Marco Di Antonio and [ ilgruppodigitale.com ]
 *
 *   The latest version of this program can be found at:
 *
 *     http://git.shaftnet.org/cgit/selphy_print.git
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "backend_common.h"

#define USB_VID_DNP       0x1343
#define USB_PID_DNP_DS40  0x0003 // Also Citizen CX
#define USB_PID_DNP_DS80  0x0004 // Also Citizen CX-W
#define USB_PID_DNP_DSRX1 0x0005 // Also Citizen CY

//#define USB_PID_CITIZEN_CW-01 XXXXX
//#define USB_PID_CITIZEN_OP900 XXXXX
//#define USB_PID_CITIZEN_CW-02 XXXXX
//#define USB_PID_CITIZEN_OP900II XXXXX
//#define USB_VID_MITSU       0x06D3
//#define USB_PID_MITSU_CP3800DW  XXXXX

/* Private data stucture */
struct dnpds40_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	int type;

	int buf_needed;
	uint32_t last_matte;

	uint8_t *qty_offset;

	uint8_t *databuf;
	int datalen;
};

struct dnpds40_cmd {
	uint8_t esc; /* Fixed at ascii ESC, aka 0x1B */
	uint8_t p;   /* Fixed at ascii 'P' aka 0x50 */
	uint8_t arg1[6];
	uint8_t arg2[16];
	uint8_t arg3[8]; /* Decimal value of arg4's length, or empty */
	uint8_t arg4[0]; /* Extra payload if arg3 is non-empty
			    Doesn't have to be sent in the same URB */

	/* All unused elements are set to 0x20 (ie ascii space) */
};

#define min(__x, __y) ((__x) < (__y)) ? __x : __y

static void dnpds40_build_cmd(struct dnpds40_cmd *cmd, char *arg1, char *arg2, uint32_t arg3_len)
{
	memset(cmd, 0x20, sizeof(*cmd));
	cmd->esc = 0x1b;
	cmd->p = 0x50;
	memcpy(cmd->arg1, arg1, min(strlen(arg1), sizeof(cmd->arg1)));
	memcpy(cmd->arg2, arg2, min(strlen(arg2), sizeof(cmd->arg2)));
	if (arg3_len) {
		char buf[9];
		snprintf(buf, sizeof(buf), "%08u", arg3_len);
		memcpy(cmd->arg3, buf, 8);
	}

}

static void dnpds40_cleanup_string(char *start, int len)
{
	char *ptr = strchr(start, 0x0d);

	if (ptr && (ptr - start < len)) {
		*ptr = 0x00; /* If there is a <CR>, terminate there */
		len = ptr - start;
	} else {
		start[--len] = 0x00;  /* force null-termination */
	}

	/* Trim trailing spaces */
	while (len && start[len-1] == ' ') {
		start[--len] = 0;
	}
}

static char *dnpds40_media_types(char *str)
{
	char tmp[4];
	int i;

	memcpy(tmp, str + 4, 3);
	tmp[3] = 0;

	i = atoi(tmp);

	switch (i) {
	case 200: return "5x3.5 (L)";
	case 210: return "5x7 (2L)";
	case 300: return "6x4 (PC)";
	case 310: return "6x8 (A5)";
	case 400: return "6x9 (A5W)";
	case 500: return "8x10";
	case 510: return "8x12";
	default:
		break;
	}

	return "Unknown type";
}

static char *dnpds40_statuses(char *str)
{
	char tmp[6];
	int i;
	memcpy(tmp, str, 5);
	tmp[5] = 0;

	i = atoi(tmp);

	switch (i) {
	case 0:	return "Idle";
	case 1:	return "Printing";
	case 500: return "Cooling Print Head";
	case 510: return "Cooling Paper Motor";
	case 1000: return "Cover Open";
	case 1010: return "No Scrap Box";
	case 1100: return "Paper End";
	case 1200: return "Ribbon End";
	case 1300: return "Paper Jam";
	case 1400: return "Ribbon Error";
	case 1500: return "Paper Definition Error";
	case 1600: return "Data Error";
	case 2000: return "Head Voltage Error";
	case 2100: return "Head Position Error";
	case 2200: return "Power Supply Fan Error";
	case 2300: return "Cutter Error";
	case 2400: return "Pinch Roller Error";
	case 2500: return "Abnormal Head Temperature";
	case 2600: return "Abnormal Media Temperature";
	case 2610: return "Abnormal Paper Motor Temperature";
	case 2700: return "Ribbon Tension Error";
	case 2800: return "RF-ID Module Error";
	case 3000: return "System Error";
	default:
		break;
	}

	return "Unkown Error";
}

static int dnpds40_do_cmd(struct dnpds40_ctx *ctx,
			  struct dnpds40_cmd *cmd,
			  uint8_t *data, int len)
{
	int ret;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*)cmd, sizeof(*cmd))))
		return ret;

	if (data && len) 
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     data, len)))
			return ret;

	return 0;
}

static uint8_t * dnpds40_resp_cmd(struct dnpds40_ctx *ctx,
				  struct dnpds40_cmd *cmd,
				  int *len)
{
	char tmp[9];
	uint8_t *respbuf;

	int ret, i, num = 0;

	memset(tmp, 0, sizeof(tmp));

	if ((ret = dnpds40_do_cmd(ctx, cmd, NULL, 0)))
		return NULL;

	/* Read in the response header */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*)tmp, 8, &num);
	if (ret < 0)
		return NULL;

	if (num != 8) {
		ERROR("Short read! (%d/%d)\n", num, 8);
		return NULL;
	}

	i = atoi(tmp);  /* Length of payload in bytes, possibly padded */
	respbuf = malloc(i);

	/* Read in the actual response */
	ret = read_data(ctx->dev, ctx->endp_up,
			respbuf, i, &num);
	if (ret < 0) {
		free(respbuf);
		return NULL;
	}

	if (num != i) {
		ERROR("Short read! (%d/%d)\n", num, i);
		free(respbuf);
		return NULL;
	}

	*len = num;
	return respbuf;
}

static int dnpds40_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	struct dnpds40_ctx ctx = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	/* Get Serial Number */
	dnpds40_build_cmd(&cmd, "INFO", "SERIAL_NUMBER", 0);

	resp = dnpds40_resp_cmd(&ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	strncpy(buf, (char*)resp, buf_len);
	buf[buf_len-1] = 0;

	free(resp);

	return 0;
}

static void *dnpds40_init(void)
{
	struct dnpds40_ctx *ctx = malloc(sizeof(struct dnpds40_ctx));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct dnpds40_ctx));

	ctx->type = P_ANY;

	return ctx;
}

static void dnpds40_attach(void *vctx, struct libusb_device_handle *dev,
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct dnpds40_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	/* Map out device type */
	if (desc.idProduct == USB_PID_DNP_DS40)
		ctx->type = P_DNP_DS40;
	else
		ctx->type = P_DNP_DS80;

}

static void dnpds40_teardown(void *vctx) {
	struct dnpds40_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

#define MAX_PRINTJOB_LEN (((2448*7536+1024+54))*3+1024) /* Worst-case */

static int dnpds40_read_parse(void *vctx, int data_fd) {
	struct dnpds40_ctx *ctx = vctx;
	int run = 1;
	char buf[9] = { 0 };

	uint32_t matte = 0, multicut = 0, dpi = 0;

	if (!ctx)
		return 1;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	/* There's no way to figure out the total job length in advance, we
	   have to parse the stream until we get to the image plane data, 
	   and even then the stream can contain arbitrary commands later.

	   So instead, we allocate a buffer of the maximum possible length, 
	   then parse the incoming stream until we hit the START command at
	   the end of the job.
	*/

	ctx->datalen = 0;
	ctx->databuf = malloc(MAX_PRINTJOB_LEN);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return 2;
	}

	while (run) {
		int remain, i, j;
		/* Read in command header */
		i = read(data_fd, ctx->databuf + ctx->datalen, 
			 sizeof(struct dnpds40_cmd));
		if (i < 0)
			return i;
		if (i == 0)
			break;
		if (i < (int) sizeof(struct dnpds40_cmd))
			return 1;

		if (ctx->databuf[ctx->datalen + 0] != 0x1b ||
		    ctx->databuf[ctx->datalen + 1] != 0x50) {
			ERROR("Unrecognized header data format @%d!\n", ctx->datalen);
			return 1;
		}

		/* Parse out length of data chunk, if any */
		memcpy(buf, ctx->databuf + ctx->datalen + 24, 8);
		j = atoi(buf);

		/* Read in data chunk as quickly as possible */
		remain = j;
		while (remain > 0) {
			i = read(data_fd, ctx->databuf + ctx->datalen + sizeof(struct dnpds40_cmd), 
				 remain);
			if (i < 0)
				return i;
			if (i == 0)
				return 1;
			ctx->datalen += i;
			remain -= i;
		}
		ctx->datalen -= j; /* Back it off */

		/* Check for some offsets */
		if(!memcmp("CNTRL QTY", ctx->databuf + ctx->datalen+2, 9)) {
			ctx->qty_offset = ctx->databuf + ctx->datalen + 32;
		}
		if(!memcmp("CNTRL OVERCOAT", ctx->databuf + ctx->datalen+2, 14)) {
			memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
			matte = atoi(buf);
		        matte = le32_to_cpu(matte);
		}
		if(!memcmp("CNTRL MULTICUT", ctx->databuf + ctx->datalen+2, 14)) {
			memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
			multicut = atoi(buf);
		        multicut = le32_to_cpu(multicut);
		}
	        if(!memcmp("IMAGE YPLANE", ctx->databuf + ctx->datalen + 2, 12)) {
			uint32_t x_ppm;
			memcpy(&x_ppm, ctx->databuf + ctx->datalen + 32 + 42, sizeof(x_ppm));
			x_ppm = le32_to_cpu(x_ppm);

			ctx->buf_needed = 1;
			dpi = 300;

			if (x_ppm == 23615) { /* pixels per meter, aka 600dpi */
				dpi = 600;
				if (ctx->type == P_DNP_DS80) { /* DS80/CX-W */
					if (matte && (multicut == 21 || // A4 length
						      multicut == 20 || // 8x4*3
						      multicut == 19 || // 8x8+8x4
						      multicut == 15 || // 8x6*2
						      multicut == 7)) // 8x12
						ctx->buf_needed = 2;
				} else { /* DS40/CX/CY/etc */
					if (multicut == 4 ||  // 6x8
					    multicut == 5 ||  // 6x9
					    multicut == 12)   // 6x4*2
						ctx->buf_needed = 2;
					else if (matte && multicut == 3) // 5x7
						ctx->buf_needed = 2;
				}

				/* If we are missing a multicut command,
				   we can't parse this job so must assume 
				   worst case size needing both buffers! */
				if (!multicut)
					ctx->buf_needed = 2;
			}
		}

		/* This is the last block.. */
	        if(!memcmp("CNTRL START", ctx->databuf + ctx->datalen + 2, 11))
			run = 0;

		/* Add in the size of this chunk */
		ctx->datalen += sizeof(struct dnpds40_cmd) + j;
	}

	/* Special case: switching to matte or back needs both buffers */
	if (matte != ctx->last_matte)
		ctx->buf_needed = 2;

	DEBUG("dpi %u matte %u(%u) mcut %u bufs %d\n", 
	      dpi, matte, ctx->last_matte, multicut, ctx->buf_needed);

	/* Track if our last print was matte */
	ctx->last_matte = matte;

	if (!ctx->datalen)
		return 1;

	return 0;
}

static int dnpds40_main_loop(void *vctx, int copies) {
	struct dnpds40_ctx *ctx = vctx;
	int ret;
	struct dnpds40_cmd cmd;
	uint8_t *resp = NULL;
	int len = 0;

	uint8_t *ptr;
	char buf[9];

	if (!ctx)
		return 1;

	/* Parse job to figure out quantity offset. */
	if (copies > 1 && ctx->qty_offset) {
		snprintf(buf, sizeof(buf), "%07d\r", copies);
		memcpy(ctx->qty_offset, buf, 8);

		// XXX should we set/reset BUFFCNTRL?
		// XXX should we verify we have sufficient media for prints?
	}

top:

	if (resp) free(resp);

	/* Query status */
	dnpds40_build_cmd(&cmd, "STATUS", "", 0);
	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;
	dnpds40_cleanup_string((char*)resp, len);

	/* If we're not idle */
	if (strcmp("00000", (char*)resp)) {
		if (!strcmp("00001", (char*)resp)) {
			free(resp);
			/* Query buffer state */
			dnpds40_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);
			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return -1;
			dnpds40_cleanup_string((char*)resp, len);

			/* Check to see if we have sufficient buffers */
			if (!strcmp("FBP00", (char*)resp) ||
			    (ctx->buf_needed == 1 && !strcmp("FBP01", (char*)resp))) {
				INFO("Insufficient printer buffers, retrying...\n");
				sleep(1);
				goto top;
			}
		} else if (!strcmp("00500", (char*)resp) ||
			   !strcmp("00510", (char*)resp)) {
			INFO("Printer cooling, retrying...\n");
			sleep(1);
			goto top;
		}
		free(resp);
		ERROR("Printer Status: %s\n", dnpds40_statuses((char*)resp));
		return 1;
	}
	
	/* Send the stream over as individual data chunks */
	ptr = ctx->databuf;

	while(ptr && ptr < (ctx->databuf + ctx->datalen)) {
		int i;
		buf[8] = 0;
		memcpy(buf, ptr + 24, 8);
		i = atoi(buf) + 32;


		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr, i)))
			return ret;

		ptr += i;
	}
	
	/* This printer handles copies internally */
	if (ctx->qty_offset)
		copies = 1;

	/* Clean up */
	if (terminate)
		copies = 1;
	
	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	if (resp) free(resp);

	return 0;
}

static int dnpds40_get_info(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Get Serial Number */
	dnpds40_build_cmd(&cmd, "INFO", "SERIAL_NUMBER", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Serial Number: '%s'\n", (char*)resp);

	free(resp);

	/* Get Firmware Version */
	dnpds40_build_cmd(&cmd, "INFO", "FVER", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Firmware Version: '%s'\n", (char*)resp);

	free(resp);

	/* Get Sensor Info */
	dnpds40_build_cmd(&cmd, "INFO", "SENSOR", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Sensor Info: '%s'\n", (char*)resp);
	// XXX parse this out. Each token is 'XXX-###' delimited by '; '

	free(resp);

	/* Get Qty of prints made on this media? */
	dnpds40_build_cmd(&cmd, "INFO", "PQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Prints Performed(?): '%s'\n", (char*)resp + 4);

	free(resp);

	/* Get Horizonal resolution */
	dnpds40_build_cmd(&cmd, "INFO", "RESOLUTION_H", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Horizontal Resolution: '%s' dpi\n", (char*)resp + 3);

	free(resp);

	/* Get Vertical resolution */
	dnpds40_build_cmd(&cmd, "INFO", "RESOLUTION_V", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Vertical Resolution: '%s' dpi\n", (char*)resp + 3);

	free(resp);

	/* Get Media Color offset */
	dnpds40_build_cmd(&cmd, "INFO", "MCOLOR", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Color Offset: '%02x%02x%02x%02x'\n", *(resp+2), *(resp+3),
	     *(resp+4), *(resp+5));

	free(resp);

	/* Get Media Lot */
	dnpds40_build_cmd(&cmd, "INFO", "MLOT", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Lot Code: '%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x'\n", 
	     *(resp+2), *(resp+3), *(resp+4), *(resp+5), *(resp+6), *(resp+7),
	     *(resp+8), *(resp+9), *(resp+10), *(resp+11), *(resp+12), *(resp+13));

	free(resp);

	/* Get Media ID Set (?) */
	dnpds40_build_cmd(&cmd, "MNT_RD", "MEDIA_ID_SET", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media ID(?): '%s'\n", (char*)resp+4);

	free(resp);

	/* Get Color Control Data Version */
	dnpds40_build_cmd(&cmd, "TBL_RD", "Version", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Color Data Version: '%s'\n", (char*)resp);

	free(resp);

	/* Get Color Control Data Checksum */
	dnpds40_build_cmd(&cmd, "MNT_RD", "CTRLD_CHKSUM", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Color Data Checksum: '%s'\n", (char*)resp);

	free(resp);


	return 0;
}

static int dnpds40_get_status(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "STATUS", "", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Printer Status: %s => %s\n", (char*)resp, dnpds40_statuses((char*)resp));

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Free Buffers: '%s'\n", (char*)resp + 3);

	free(resp);

	/* Get Media Info */
	dnpds40_build_cmd(&cmd, "INFO", "MEDIA", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Type: '%s'\n", dnpds40_media_types((char*)resp));

#if 0
	switch (*(resp+3)) {
	case '1':
		INFO("   Stickier paper\n");
		break;
	case '0':
		INFO("   Standard paper\n");
		break;
	default:
		INFO("   Unknown paper(%c)\n", *(resp+4));
		break;
	}
	switch (*(resp+6)) {
	case '1':
		INFO("   With mark\n");
		break;
	case '0':
		INFO("   Without mark\n");
		break;
	default:
		INFO("   Unknown mark(%c)\n", *(resp+7));
		break;
	}
#endif

	free(resp);

	/* Get Media remaining */
	dnpds40_build_cmd(&cmd, "INFO", "MQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Prints Remaining: '%s'\n", (char*)resp + 4);

	free(resp);

	return 0;
}

static int dnpds40_get_counters(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_LIFE", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Lifetime Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_A", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("A Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_B", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("B Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_P", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("P Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_M", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("M Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_MATTE", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Matte Counter: '%s'\n", (char*)resp+4);

	free(resp);

	return 0;
}

static int dnpds40_clear_counter(struct dnpds40_ctx *ctx, char counter)
{
	struct dnpds40_cmd cmd;
	char msg[4];
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_WT", "COUNTER_CLEAR", 4);
	msg[0] = 'C';
	msg[1] = counter;
	msg[2] = 0x0d; /* ie carriage return, ASCII '\r' */
	msg[3] = 0x00;

	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)msg, 4)))
		return ret;

	return 0;
}

static int dnpds40_set_counter_p(struct dnpds40_ctx *ctx, char *arg)
{
	struct dnpds40_cmd cmd;
	char msg[9];
	int i = atoi(arg);
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_WT", "COUNTERP_SET", 8);
	snprintf(msg, sizeof(msg), "%08d", i);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)msg, 8)))
		return ret;

	return 0;
}

static void dnpds40_cmdline(void)
{
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -n ]           # Query counters\n");
	DEBUG("\t\t[ -N A|B|M ]     # Clear counter A/B/M\n");
	DEBUG("\t\t[ -p num ]       # Set counter P\n");

}

static int dnpds40_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct dnpds40_ctx *ctx = vctx;
	int i, j = 0;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, "inN:p:s")) >= 0) {
		switch(i) {
		case 'i':
			if (ctx) {
				j = dnpds40_get_info(ctx);
				break;
			}
			return 1;
		case 'n':
			if (ctx) {
				j = dnpds40_get_counters(ctx);
				break;
			}
			return 1;
		case 'N':
			if (optarg[0] != 'A' &&
			    optarg[0] != 'B' &&
			    optarg[0] != 'M')
				return -1;
			if (ctx) {
				j = dnpds40_clear_counter(ctx, optarg[0]);
				break;
			}
			return 1;
		case 'p':
			if (ctx) {
				j = dnpds40_set_counter_p(ctx, optarg);
				break;
			}
			return 1;
		case 's':
			if (ctx) {
				j = dnpds40_get_status(ctx);
				break;
			}
			return 1;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

/* Exported */
struct dyesub_backend dnpds40_backend = {
	.name = "DNP DS40/DS80/DSRX1",
	.version = "0.29",
	.uri_prefix = "dnpds40",
	.cmdline_usage = dnpds40_cmdline,
	.cmdline_arg = dnpds40_cmdline_arg,
	.init = dnpds40_init,
	.attach = dnpds40_attach,
	.teardown = dnpds40_teardown,
	.read_parse = dnpds40_read_parse,
	.main_loop = dnpds40_main_loop,
	.query_serno = dnpds40_query_serno,
	.devices = {
	{ USB_VID_DNP, USB_PID_DNP_DS40, P_DNP_DS40, ""},
	{ USB_VID_DNP, USB_PID_DNP_DS80, P_DNP_DS80, ""},
	{ USB_VID_DNP, USB_PID_DNP_DSRX1, P_DNP_DS40, ""},
//	{ USB_VID_CITIZEN, USB_PID_CITIZEN_CW-02, P_DNP_DS40, ""},
//	{ USB_VID_CITIZEN, USB_PID_CITIZEN_OP900II, P_DNP_DS40, ""},
//	{ USB_VID_CITIZEN, USB_PID_CITIZEN_CW-01, P_DNP_DS40, ""},
//	{ USB_VID_CITIZEN, USB_PID_CITIZEN_OP900, P_DNP_DS40, ""},
//	{ USB_VID_MITSU, USB_PID_MITSU_CP38000W, P_DNP_DS80, ""},
	{ 0, 0, 0, ""}
	}
};
