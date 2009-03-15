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
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "config.h"
#include "debug.h"
#include "icon.h"
#include "temperhum.h"
#include "comms.h"
#include "tray.h"

#include "digits.h"
#include "connecting.xbm"
#include "not_connected.xbm"

void tray_init(struct th_data *data) {
	data->status.conn = NOT_CONNECTED;
	data->tray_ok = 0;
}

void tray_add(HWND hwnd, struct th_data *data) {
	NOTIFYICONDATA *niData;
	BOOL ret;
	DWORD err;

	odprintf("tray[add]");

	if (!data->tray_ok) {
		niData = &data->niData;
		niData->cbSize = sizeof(NOTIFYICONDATA);
		niData->hWnd = hwnd;
		niData->uID = TRAY_ID;
		niData->uFlags = NIF_MESSAGE|NIF_TIP;
		niData->uCallbackMessage = WM_APP_TRAY;
		niData->hIcon = NULL;
		niData->szTip[0] = 0;
		niData->uVersion = NOTIFYICON_VERSION;

		SetLastError(0);
		ret = Shell_NotifyIcon(NIM_ADD, niData);
		err = GetLastError();
		odprintf("Shell_NotifyIcon[ADD]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
		if (ret == TRUE)
			data->tray_ok = 1;

		SetLastError(0);
		ret = Shell_NotifyIcon(NIM_SETVERSION, niData);
		err = GetLastError();
		odprintf("Shell_NotifyIcon[SETVERSION]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
		if (ret != TRUE)
			niData->uVersion = 0;
	}
}

void tray_remove(HWND hwnd, struct th_data *data) {
	NOTIFYICONDATA *niData = &data->niData;
	BOOL ret;
	DWORD err;
	(void)hwnd;

	odprintf("tray[remove]");

	if (data->tray_ok) {
		SetLastError(0);
		ret = Shell_NotifyIcon(NIM_DELETE, niData);
		err = GetLastError();
		odprintf("Shell_NotifyIcon[DELETE]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
		if (ret == TRUE)
			data->tray_ok = 0;
	}
}

void tray_update(HWND hwnd, struct th_data *data) {
	struct tray_status *status = &data->status;
	NOTIFYICONDATA *niData = &data->niData;
	unsigned int fg, bg, p, d;
	BOOL ret;
	DWORD err;

	if (!data->tray_ok) {
		tray_add(hwnd, data);

		if (!data->tray_ok)
			return;
	}

	odprintf("tray[update]: conn=%d error=\"%s\" temperature_celsius=%f relative_humidity=%f dew_point=%f",
		status->conn, status->error, status->temperature_celsius, status->relative_humidity, status->dew_point);

	fg = 0;
	bg = ~0;

	switch (status->conn) {
	case NOT_CONNECTED:
		if (not_connected_width < ICON_WIDTH || not_connected_height < ICON_HEIGHT)
			icon_wipe(bg);

		icon_blit(fg, bg, 0, 0, 0, 0, 0, not_connected_width, not_connected_height, not_connected_bits);

		if (status->error[0] != 0)
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Not Connected: %s", status->error);
		else
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Not Connected");
		if (ret < 0)
			niData->szTip[0] = 0;
		break;

	case CONNECTING:
		if (connecting_width < ICON_WIDTH || connecting_height < ICON_HEIGHT)
			icon_wipe(bg);

		icon_blit(fg, bg, 0, 0, 0, 0, 0, connecting_width, connecting_height, connecting_bits);

		if (status->error[0] != 0)
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Connecting to %s", status->error);
		else
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Connecting");
		if (ret < 0)
			niData->szTip[0] = 0;
		break;

	case CONNECTED:
		if (isnan(status->temperature_celsius)) {
			icon_clear(0, 0, bg, 0, 0, ICON_WIDTH, ICON_HEIGHT/2);
			p = 0;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dot_width, digit_dot_height, digit_dot_bits);
			p += digit_dot_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, 0, digit_dash_width, digit_dash_height, digit_dash_bits);
		} else {
			unsigned int h_fg, h_bg, h_end;
			int tc = lrint(status->temperature_celsius * 100.0);

			if (tc > 99999)
				tc = 99999;
			if (tc < -9999)
				tc = -9999;

			h_end = ICON_WIDTH/2;
			h_fg = bg;
			h_bg = fg;

			icon_clear(h_bg, h_end, bg, 0, 0, ICON_WIDTH, ICON_HEIGHT/2);

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
			icon_clear(0, 0, bg, 0, ICON_HEIGHT/2, ICON_WIDTH, ICON_HEIGHT/2);
			p = 0;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dash_width, digit_dash_height, digit_dash_bits);
			p += digit_dash_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dot_width, digit_dot_height, digit_dot_bits);
			p += digit_dot_width + 1;

			icon_blit(fg, bg, 0, 0, 0, p, ICON_HEIGHT/2, digit_dash_width, digit_dash_height, digit_dash_bits);
		} else {
			unsigned int h_fg, h_bg, h_end;
			int rh = lrint(status->relative_humidity * 10.0);

			if (rh > 999)
				rh = 999;
			if (rh < 0)
				rh = 0;

			h_end = ICON_WIDTH/2;
			h_fg = bg;
			h_bg = fg;

			icon_clear(h_bg, h_end, bg, 0, ICON_HEIGHT/2, ICON_WIDTH, ICON_HEIGHT/2);

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

		snprintf(niData->szTip, sizeof(niData->szTip), "Temperature: %.2fC, Relative Humidity: %.2f%%, Dew Point: %.2fC",
			status->temperature_celsius, status->relative_humidity, status->dew_point);
		break;

	default:
		return;
	}

	niData->uFlags |= NIF_ICON;

	SetLastError(0);
	ret = Shell_NotifyIcon(NIM_MODIFY, niData);
	err = GetLastError();
	odprintf("Shell_NotifyIcon[MODIFY]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
	if (ret != TRUE)
		tray_remove(hwnd, data);
}

BOOL tray_activity(HWND hwnd, struct th_data *data, WPARAM wParam, LPARAM lParam) {
	(void)hwnd;

	odprintf("tray[activity]: wParam=%ld lParam=%ld", wParam, lParam);

	if (wParam != TRAY_ID)
		return FALSE;

	switch (data->niData.uVersion) {
		case NOTIFYICON_VERSION:
			switch (lParam) {
			case WM_CONTEXTMENU:
				temperhum_shutdown(data, EXIT_SUCCESS);
				return TRUE;

			default:
				return FALSE;
			}

		case 0:
			switch (lParam) {
			case WM_RBUTTONUP:
				temperhum_shutdown(data, EXIT_SUCCESS);
				return TRUE;

			default:
				return FALSE;
			}

	default:
		return FALSE;
	}
}
