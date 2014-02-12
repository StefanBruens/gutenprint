/*
 *   Sony UP-DR150 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2014 Solomon Peachy <pizza@shaftnet.org>
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

/* Private data stucture */
struct updr150_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	uint8_t *databuf;
	int datalen;
};

static void* updr150_init(void)
{
	struct updr150_ctx *ctx = malloc(sizeof(struct updr150_ctx));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct updr150_ctx));
	return ctx;
}

static void updr150_attach(void *vctx, struct libusb_device_handle *dev, 
			   uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct updr150_ctx *ctx = vctx;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;
}

static void updr150_teardown(void *vctx) {
	struct updr150_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

#define MAX_PRINTJOB_LEN 16736455
static int updr150_read_parse(void *vctx, int data_fd) {
	struct updr150_ctx *ctx = vctx;
	int len, run = 1;

	if (!ctx)
		return 1;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	ctx->datalen = 0;
	ctx->databuf = malloc(MAX_PRINTJOB_LEN);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return 2;
	}

	while(run) {
		int i;
		int keep = 0;
		i = read(data_fd, ctx->databuf + ctx->datalen, 4);
		if (i < 0)
			return i;
		if (i == 0)
			break;

		memcpy(&len, ctx->databuf + ctx->datalen, sizeof(len));
		len = le32_to_cpu(len);

		/* Filter out chunks we don't send to the printer */
		switch (len) {
		case 0xffffff6a:
		case 0xfffffffc:
		case 0xfffffffb:
		case 0xfffffff4:
		case 0xffffffed:
		case 0xfffffff9:
		case 0xfffffff8:
		case 0xffffffec:
		case 0xffffffeb:
		case 0xfffffffa:
			if(dyesub_debug)
				DEBUG("Block ID '%08x' (len %d)\n", len, 0);
			len = 0;
			break;
		case 0xfffffff3:
			if(dyesub_debug)
				DEBUG("Block ID '%08x' (len %d)\n", len, 0);
			len = 0;
			run = 0;
			break;
		case 0xffffffef:
		case 0xfffffff5:
			if(dyesub_debug)
				DEBUG("Block ID '%08x' (len %d)\n", len, 4);
			len = 4;
			break;
		default:
			if (len & 0xff000000) {
				ERROR("Unknown block ID '%08x', aborting!\n", len);
				return 1;
			} else {
				/* Only keep these chunks */
				if(dyesub_debug)
					DEBUG("Data block (len %d)\n", len);
				keep = 1;
			}
			break;
		}
		if (keep)
			ctx->datalen += sizeof(uint32_t);

		/* Read in the data chunk */
		while(len > 0) {
			i = read(data_fd, ctx->databuf + ctx->datalen, len);
			if (i < 0)
				return i;
			if (i == 0)
				break;
			if (keep)
				ctx->datalen += i;
			len -= i;
		}
	}
	if (!ctx->datalen)
		return 1;

	return 0;
}

static int updr150_main_loop(void *vctx, int copies) {
	struct updr150_ctx *ctx = vctx;
	int i = 0, ret;

	if (!ctx)
		return 1;

top:
	while (i < ctx->datalen) {
		uint32_t len;
		memcpy(&len, ctx->databuf + i, sizeof(len));
		len = le32_to_cpu(len);

		i += sizeof(uint32_t);

		if (dyesub_debug)
			DEBUG("Sending %u bytes to printer @ %i\n", len, i);
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf + i, len)))
			return ret;

		i += len;
	}

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	return 0;
}

/* Exported */
#define USB_VID_SONY         0x054C
#define USB_PID_SONY_UPDR150 0x01E8
#define USB_PID_SONY_UPDR200 0x035F

struct dyesub_backend updr150_backend = {
	.name = "Sony UP-DR150/UP-DR200",
	.version = "0.13",
	.uri_prefix = "sonyupdr150",
	.init = updr150_init,
	.attach = updr150_attach,
	.teardown = updr150_teardown,
	.read_parse = updr150_read_parse,
	.main_loop = updr150_main_loop,
	.devices = {
	{ USB_VID_SONY, USB_PID_SONY_UPDR150, P_SONY_UPDR150, ""},
	{ USB_VID_SONY, USB_PID_SONY_UPDR200, P_SONY_UPDR150, ""},
	{ 0, 0, 0, ""}
	}
};

/* Sony UP-DR150/UP-DR200 Spool file format

   The spool file is a series of 4-byte commands, followed by optional
   arguments.  The purpose of the commands is unknown, but they presumably
   instruct the driver to perform certain things.

   If you treat these 4 bytes as a 32-bit little-endian number, if the most significant
   four bits are bits are non-zero, the value is is to be interpreted as a driver
   command.  If the most significant bits are zero, the value signifies that the following
   N bytes of data should be sent to the printer as-is.

   Known driver "commands":

   6a ff ff ff
   fc ff ff ff
   fb ff ff ff
   f4 ff ff ff
   ed ff ff ff
   f9 ff ff ff
   f8 ff ff ff
   ec ff ff ff
   eb ff ff ff
   fa ff ff ff
   f3 ff ff ff
   ef ff ff ff  XX 00 00 00   # XX == print size (0x01/0x02/0x03/0x04)
   f5 ff ff ff  YY 00 00 00   # YY == ??? (seen 0x01)

   All printer commands start with 0x1b, and are at least 7 bytes long.

  ************************************************************************

  The data stream sent to the printer consists of all the commands in the
  spool file, plus a couple other ones that generate responses.  It is
  unknown if those additional commands are necessary.  This is a typical
  sequence:

[[ Sniff start of a UP-DR150 ]]

<- 1b e0 00 00 00 0f 00
-> 0e 00 00 00 00 00 00 00  00 04 a8 08 0a a4 00

<- 1b 16 00 00 00 00 00
-> "reset" ??

[[ begin job ]]

<- 1b ef 00 00 00 06 00
-> 05 00 00 00 00 22

<- 1b e5 00 00 00 08 00       ** In spool file
<- 00 00 00 00 00 00 01 00

<- 1b c1 00 02 06 00 00
-> 02 02 00 03 00 00

<- 1b ee 00 00 00 02 00       ** In spool file
<- 00 01

<- 1b 15 00 00 00 0d 00       ** In spool file
<- 00 00 00 00 07 00 00 00  00 08 00 0a a4

<- 1b 03 00 00 00 13 00
-> 70 00 00 00 00 00 00 0b  00 00 00 00 00 00 00 00
   00 00 00

<- 1b e1 00 00 00 0b 00       ** In spool file
<- 00 80 00 00 00 00 00 08  00 0a a4

<- 1b 03 00 00 00 13 00
-> 70 00 00 00 00 00 00 0b  00 00 00 00 00 00 00 00
   00 00 00

<- 1b ea 00 00 00 00 00 ff  60 00 00   ** In spool file
<- [[ 0x0060ff00 bytes of data ]]

<- 1b e0 00 00 00 0f 00
-> 0e 00 00 00 00 00 00 00  04 a8 08 00 0a a4 00

<- 1b 0a 00 00 00 00 00   ** In spool file
<- 1b 17 00 00 00 00 00   ** In spool file

[[fin]]

*/
