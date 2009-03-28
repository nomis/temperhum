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

#define TRAY_ID 1

int tray_init(struct th_data *data);
void tray_reset(HWND hWnd, struct th_data *data);
void tray_add(HWND hWnd, struct th_data *data);
void tray_update(HWND hWnd, struct th_data *data);
BOOL tray_activity(HWND hWnd, struct th_data *data, WPARAM wParam, LPARAM lParam);
void tray_remove(HWND hWnd, struct th_data *data);
