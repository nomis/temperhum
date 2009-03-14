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

#define ICON_WIDTH 16
#define ICON_HEIGHT 16
#define ICON_DEPTH_BITS 24
#define ICON_DEPTH_BYTES (ICON_DEPTH_BITS >> 3)

void icon_blit(unsigned int fg, unsigned int bg, unsigned int cx, unsigned int fg2, unsigned int bg2, unsigned int sx, unsigned int sy, unsigned int width, unsigned int height, const unsigned char *data);
void icon_wipe(unsigned int bg);
void icon_clear(unsigned int bg1, unsigned int cx, unsigned int bg2, unsigned int sx, unsigned int sy, unsigned int width, unsigned int height);
