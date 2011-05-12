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

#define ICON_WIDTH 16
#define ICON_HEIGHT 16
#define ICON_DEPTH_BITS 24
#define ICON_DEPTH_BYTES (ICON_DEPTH_BITS >> 3)
#define ICON_MASK_SCANLINE_BYTES ((ICON_WIDTH + 15) >> 3 & ~1)
#define ICON_DATA_SCANLINE_BYTES ((ICON_WIDTH * ICON_DEPTH_BITS + 15) >> 3 & ~1)

HICON icon_create(void);
void icon_destroy(HICON hIcon);
void icon_blit(unsigned int fg, unsigned int bg, unsigned int cx, unsigned int fg2, unsigned int bg2, unsigned int sx, unsigned int sy, unsigned int width, unsigned int height, const unsigned char *data);
void icon_wipe(unsigned int bg);
void icon_clear(unsigned int bg1, unsigned int cx, unsigned int bg2, unsigned int sx, unsigned int sy, unsigned int width, unsigned int height);
int icon_init(void);
void icon_free(void);
unsigned int icon_syscolour(int element);
