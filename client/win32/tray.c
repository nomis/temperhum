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
#include "tray.h"

#include "digit_dash.xbm"
#include "digit_dot.xbm"
#include "digit_zero.xbm"
#include "digit_one.xbm"
#include "digit_two.xbm"
#include "digit_three.xbm"
#include "digit_four.xbm"
#include "digit_five.xbm"
#include "digit_six.xbm"
#include "digit_seven.xbm"
#include "digit_eight.xbm"
#include "digit_nine.xbm"

char icon_buf[16*16];

void update_tray(struct tray_status *status) {
	odprintf("tray: conn=%d, temperature_celsius=%f, relative_humidity=%f, dew_point=%f",
		status->conn, status->temperature_celsius, status->relative_humidity, status->dew_point);
}
