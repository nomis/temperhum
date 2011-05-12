/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>

#include "debug.h"
#include "icon.h"

static HBITMAP hbmMask;
static unsigned char icon_and[ICON_HEIGHT * ICON_MASK_SCANLINE_BYTES];
static unsigned char icon_buf[ICON_HEIGHT * ICON_DATA_SCANLINE_BYTES];

static inline void icon_set(unsigned int x, unsigned int y, unsigned int c) {
	unsigned int d;

	for (d = 0; d < ICON_DEPTH_BYTES; d++)
		icon_buf[y * ICON_DATA_SCANLINE_BYTES + x * ICON_DEPTH_BYTES + d] = (c >> (d << 3)) & 0xFF;
}

static inline unsigned int icon_bpp_adjust(unsigned int c) {
#if ICON_DEPTH_BITS == 32
	/* Ignore transparency */
	return c & 0xff000000;
#elif ICON_DEPTH_BITS == 24
	return c;
#elif ICON_DEPTH_BITS == 16
	return ((c >> 9) & 0x7c00) | ((c >> 5) & 0x7c0) | (c >> 3);
#else
#	error "Unable to handle icon bit depth"
#endif
}

int icon_init(void) {
	DWORD err;

	odprintf("icon[init]");

	memset(icon_and, 0, sizeof(icon_and));

	SetLastError(0);
	hbmMask = CreateBitmap(ICON_WIDTH, ICON_HEIGHT, 1, 1, icon_and);
	err = GetLastError();
	odprintf("CreateBitmap: %p (%ld)", hbmMask, err);

	if (hbmMask == NULL)
		return 1;
	return 0;
}

void icon_free(void) {
	BOOL retb;
	DWORD err;

	odprintf("icon[free]");

	SetLastError(0);
	retb = DeleteObject(hbmMask);
	err = GetLastError();
	odprintf("DeleteObject: %s (%ld)", retb == TRUE ? "TRUE" : "FALSE", err);
}

HICON icon_create(void) {
	HDC hdc, hdcMem;
	HBITMAP hbmOld, hbmIcon;
	BITMAPINFO bmi;
	ICONINFO iinfo;
	HICON icon = NULL;
	BOOL retb;
	INT ret;
	DWORD err;

	odprintf("icon[create]");

	SetLastError(0);
	hdc = GetDC(NULL);
	err = GetLastError();
	odprintf("GetDC: %p (%ld)", hdc, err);
	if (hdc == NULL)
		goto failed;

	SetLastError(0);
	hdcMem = CreateCompatibleDC(hdc);
	err = GetLastError();
	odprintf("CreateCompatibleDC: %p (%ld)", hdcMem, err);
	if (hdcMem == NULL)
		goto release_hdc;

	SetLastError(0);
	hbmIcon = CreateCompatibleBitmap(hdc, ICON_WIDTH, ICON_HEIGHT);
	err = GetLastError();
	odprintf("CreateCompatibleBitmap: %p (%ld)", hbmIcon, err);
	if (hbmIcon == NULL)
		goto delete_hdcMem;

	SetLastError(0);
	hbmOld = SelectObject(hdcMem, hbmIcon);
	err = GetLastError();
	odprintf("SelectObject: %p (%ld)", hbmOld, err);
	if (hbmOld == NULL)
		goto delete_hbmIcon;

	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = ICON_WIDTH;
	bmi.bmiHeader.biHeight = -ICON_HEIGHT;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = ICON_DEPTH_BITS;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = sizeof(icon_buf);
	bmi.bmiHeader.biXPelsPerMeter = 0; /* Per metre? That's a lot of pixels... */
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;

	/* "bmiColours" should be set to "NULL" according to MSDN. Thanks. */
	bmi.bmiColors[0].rgbBlue = 0;
	bmi.bmiColors[0].rgbGreen = 0;
	bmi.bmiColors[0].rgbRed = 0;
	bmi.bmiColors[0].rgbReserved = 0;

	SetLastError(0);
	ret = SetDIBits(hdcMem, hbmIcon, 0, ICON_HEIGHT, icon_buf, &bmi, DIB_RGB_COLORS);
	err = GetLastError();
	odprintf("SetDIBits: %d (%ld)", ret, err);

	SetLastError(0);
	hbmOld = SelectObject(hdcMem, hbmOld);
	err = GetLastError();
	odprintf("SelectObject: %p (%ld)", hbmOld, err);

	if (ret != ICON_HEIGHT)
		goto delete_hbmIcon;

	iinfo.fIcon = TRUE;
	iinfo.xHotspot = 0;
	iinfo.yHotspot = 0;
	iinfo.hbmMask = hbmMask;
	iinfo.hbmColor = hbmIcon;

	SetLastError(0);
	icon = CreateIconIndirect(&iinfo);
	err = GetLastError();
	odprintf("CreateIconIndirect: %p (%ld)", icon, err);

delete_hbmIcon:
	SetLastError(0);
	retb = DeleteObject(hbmIcon);
	err = GetLastError();
	odprintf("DeleteObject: %s (%ld)", retb == TRUE ? "TRUE" : "FALSE", err);

delete_hdcMem:
	SetLastError(0);
	retb = DeleteDC(hdcMem);
	err = GetLastError();
	odprintf("DeleteDC: %s (%ld)", retb == TRUE ? "TRUE" : "FALSE", err);

release_hdc:
	SetLastError(0);
	ret = ReleaseDC(NULL, hdc);
	err = GetLastError();
	odprintf("ReleaseDC: %d (%ld)", ret, err);

failed:	
	return icon;
}

void icon_destroy(HICON hIcon) {
	BOOL retb;
	DWORD err;

	odprintf("icon[destroy]: %p", hIcon);

	SetLastError(0);
	retb = DestroyIcon(hIcon);
	err = GetLastError();
	odprintf("DestroyIcon: %s (%ld)", retb == TRUE ? "TRUE" : "FALSE", err);

	/* if it failed, am I supposed to keep it around forever and try again? */
}

void icon_blit(unsigned int fg1, unsigned int bg1, unsigned int cx, unsigned int fg2, unsigned int bg2, unsigned int sx, unsigned int sy, unsigned int width, unsigned int height, const unsigned char *data) {
	unsigned int fg, bg;
	unsigned int x, y;
	unsigned int row_b, col_b;
	unsigned int c, px, py;

	odprintf("icon[blit]: fg1=#%08x bg1=#%08x cx=%u fg2=#%08x bg2=#%08x sx=%u sy=%u width=%u height=%u data=%p", fg1, bg1, cx, fg2, bg2, sx, sy, width, height, data);

	/* row width in bytes */
	row_b = (width + 7) >> 3;

	/* adjust colours for bpp */
	fg1 = icon_bpp_adjust(fg1);
	bg1 = icon_bpp_adjust(bg1);
	fg2 = icon_bpp_adjust(fg2);
	bg2 = icon_bpp_adjust(bg2);

	/* use first colour scheme */
	fg = fg1;
	bg = bg1;

	for (x = sx; x < sx+width && x < ICON_WIDTH; x++) {
		/* byte position in row */
		col_b = ((x - sx) >> 3);

		/* bit position in byte */
		px = ((x - sx) & 7);

		/* change to second colour scheme */
		if (x >= cx) {
			fg = fg2;
			bg = bg2;
		}
		
		for (y = sy; y < sy+height && y < ICON_HEIGHT; y++) {
			/* row offset + column offset */
			py = (row_b * (y - sy)) + col_b;

			/* select bit from row/column byte */
			if ((data[py] >> px) & 1)
				c = fg;
			else
				c = bg;

			icon_set(x, y, c);
		}
	}
}

void icon_wipe(unsigned int bg) {
	odprintf("icon[wipe]: bg=#%08x", bg);

	/* adjust colour for bpp */
	bg = icon_bpp_adjust(bg);

	icon_clear(0, 0, bg, 0, 0, ICON_WIDTH, ICON_HEIGHT);
}

void icon_clear(unsigned int bg1, unsigned int cx, unsigned int bg2, unsigned int sx, unsigned int sy, unsigned int width, unsigned int height) {
	unsigned int x, y, bg;

	odprintf("icon[clear]: bg1=#%08x cx=%u bg2=#%08x sx=%u sy=%u width=%u height=%u", bg1, cx, bg2, sx, sy, width, height);

	/* adjust colours for bpp */
	bg1 = icon_bpp_adjust(bg1);
	bg2 = icon_bpp_adjust(bg2);

	/* use first colour scheme */
	bg = bg1;

	for (x = sx; sx < sx+width && x < ICON_WIDTH; x++) {
		/* change to second colour scheme */
		if (x >= cx)
			bg = bg2;

		for (y = sy; sy < sy+height && y < ICON_HEIGHT; y++)
			icon_set(x, y, bg);
	}
}

unsigned int icon_syscolour(int element) {
	DWORD rgb = GetSysColor(element);
	return (0xff << 24) | (GetRValue(rgb) << 16) | (GetGValue(rgb) << 8) | GetBValue(rgb);
}
