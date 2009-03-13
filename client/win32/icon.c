/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "debug.h"
#include "icon.h"

static char icon_buf[ICON_WIDTH * ICON_HEIGHT * ICON_DEPTH_BYTES];

static inline void icon_set(unsigned int x, unsigned int y, unsigned int c) {
	unsigned int d;

	for (d = 0; d < ICON_DEPTH_BYTES; d++)
		icon_buf[y * (ICON_WIDTH * ICON_DEPTH_BYTES) + x * ICON_DEPTH_BYTES + d] = (c >> (ICON_DEPTH_BITS - ((d + 1) << 3))) & 0xFF;
}

void icon_blit(unsigned int fg1, unsigned int bg1, unsigned int cx, unsigned int fg2, unsigned int bg2, unsigned int sx, unsigned int sy, unsigned int width, unsigned int height, const unsigned char *data) {
	unsigned int fg, bg;
	unsigned int x, y;
	unsigned int row_b, col_b;
	unsigned int c, px, py;

	odprintf("icon_blit: fg1=#%08x bg1=#%08x cx=%u fg2=#%08x bg2=#%08x sx=%u sy=%u width=%u height=%u data=%p", fg1, bg1, cx, fg2, bg2, sx, sy, width, height, data);

	row_b = (width + ~(width & 7) + 1) >> 3;

	fg = fg1;
	bg = bg1;

	for (x = sx; x < sx+width && x < ICON_WIDTH; x++) {
		px = 8 - ((width - (x - sx)) & 7);
		col_b = ((width - (x - sx)) >> 3);

		if (x == cx) {
			fg = fg2;
			bg = bg2;
		}
		
		for (y = sy; y < sy+height && y < ICON_HEIGHT; y++) {
			py = (row_b * (height - (y - sy))) + col_b;
			
			if ((data[py] >> px) & 1)
				c = fg;
			else
				c = bg;

			icon_set(x, y, c);
		}
	}
}

void icon_wipe(unsigned int bg) {
	unsigned int x, y;

	odprintf("icon_wipe: bg=#%08x", bg);

	for (x = 0; x < ICON_WIDTH; x++)
		for (y = 0; y < ICON_HEIGHT; y++)
			icon_set(x, y, bg);
}
