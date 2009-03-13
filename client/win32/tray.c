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

#include <math.h>
#include <stdlib.h>

#include "debug.h"
#include "icon.h"
#include "tray.h"

#include "digits.h"
#include "connecting.xbm"
#include "not_connected.xbm"

void update_tray(struct tray_status *status) {
	unsigned int h_fg, h_bg, h_end, fg, bg, p, d;

	odprintf("tray: conn=%d, temperature_celsius=%f, relative_humidity=%f, dew_point=%f",
		status->conn, status->temperature_celsius, status->relative_humidity, status->dew_point);

	fg = 0;
	bg = ~0;

	h_end = ICON_WIDTH/2;
	h_fg = ~0;
	h_bg = 0;

	switch (status->conn) {
	case NOT_CONNECTED:
		if (not_connected_width < ICON_WIDTH || not_connected_height < ICON_HEIGHT)
			icon_wipe(bg);

		icon_blit(fg, bg, 0, 0, 0, 0, 0, not_connected_width, not_connected_height, not_connected_bits);
		break;

	case CONNECTING:
		if (connecting_width < ICON_WIDTH || connecting_height < ICON_HEIGHT)
			icon_wipe(bg);

		icon_blit(fg, bg, 0, 0, 0, 0, 0, connecting_width, connecting_height, connecting_bits);
		break;

	case CONNECTED:
		icon_wipe(bg);

		if (isnan(status->temperature_celsius)) {
			p = 0;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dot_width, digit_dot_height, digit_dot_bits);
			p += digit_dot_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
		} else {
			int tc = lrint(status->temperature_celsius * 100.0);
			if (tc > 99999)
				tc = 99999;
			if (tc < -9999)
				tc = -9999;

			if (tc > 9999) {
				/* _NNN 100 to 999 */
				p = digit_dot_width + 1;

				d = (tc/10000) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				d = (tc/1000) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				d = (tc/100) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
			} else if (tc > 999) {
				/* NN.N 10.0 to 99.9 */
				p = 0;

				d = (tc/1000) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				d = (tc/100) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digit_dot_width, digit_dot_height, digit_dot_bits);
				p += digit_dot_width + 1;

				d = (tc/10) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
			} else if (tc >= 0) {
				/* N.NN 0.00 to 9.99 */
				p = 0;

				d = (tc/100) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digit_dot_width, digit_dot_height, digit_dot_bits);
				p += digit_dot_width + 1;

				d = (tc/10) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				d = tc % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
			} else if (tc >= -999) { 
				/* -N.N -0.1 to -9.9 */
				p = 0;

				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
				p += digit_dash_width + 1;

				d = abs(tc/100) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digit_dot_width, digit_dot_height, digit_dot_bits);
				p += digit_dot_width + 1;

				d = abs(tc/10) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
			} else /* if (tc >= -9999) */ {
				/* _-NN -10 to -99 */
				p = digit_dot_width + 1;

				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
				p += digit_dash_width + 1;

				d = abs(tc/1000) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
				p += digits_width[d] + 1;

				d = abs(tc/100) % 10;
				icon_blit(h_fg, h_bg, h_end, fg, bg, p, 0, digits_width[d], digits_height[d], digits_bits[d]);
			}
		}

		if (isnan(status->relative_humidity)) {
			p = 0;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dot_width, digit_dot_height, digit_dot_bits);
			p += digit_dot_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dash_width, digit_dash_height, digit_dash_bits);
		} else {
			int rh = lrint(status->relative_humidity * 10.0);
			if (rh > 999)
				rh = 999;
			if (rh < 0)
				rh = 0;

			/* NN.N 00.0 to 99.9 */
			p = 0;

			d = (rh/100) % 10;
			icon_blit(h_fg, h_bg, h_end, fg, bg, p, ICON_HEIGHT/2, digits_width[d], digits_height[d], digits_bits[d]);
			p += digits_width[d] + 1;

			d = (rh/10) % 10;
			icon_blit(h_fg, h_bg, h_end, fg, bg, p, ICON_HEIGHT/2, digits_width[d], digits_height[d], digits_bits[d]);
			p += digits_width[d] + 1;

			icon_blit(h_fg, h_bg, h_end, fg, bg, p, ICON_HEIGHT/2, digit_dot_width, digit_dot_height, digit_dot_bits);
			p += digit_dot_width + 1;

			d = rh % 10;
			icon_blit(h_fg, h_bg, h_end, fg, bg, p, ICON_HEIGHT/2, digits_width[d], digits_height[d], digits_bits[d]);
		}
		break;

	default:
		return;
	}
}
