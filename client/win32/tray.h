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

#define COLOUR_WHITE 0xffffffff
#define COLOUR_BLACK 0x00000000

/* These ranges should really be defined by the
 * server, as it knows where the sensor(s) are
 * located. The assumption here is for an indoor
 * sensor.
 */

/* Health and Safety Executive (GBR)
 *
 * Temperature -> Thermal comfort -> FAQs
 * What is the minimum/maximum temperature in the workplace?
 * ---------------------------------------------------------
 *
 * The Workplace (Health, Safety and Welfare) Regulations 1992,
 * Approved Code of Practise states:
 *
 * Minimum temperature: 16℃ (13℃ for "severe physical effort")
 * Maximum temperature: undefined
 *
 * The previous HSE definition was: 13℃ to 30℃ is "acceptable"
 * 
 * http://www.hse.gov.uk/temperature/thermal/faq.htm
 * Last-Modified: 2009-02-03
 * Retrieved:     2009-03-29
 * Copyright ©2009  Crown Copyright
 */
#define MIN_TEMPC 13.0
#define MIN_TEMPC_WARN 16.0
#define MAX_TEMPC_WARN 27.0
#define MAX_TEMPC 30.0

/* Wikimedia Foundation, Inc. (USA)
 *
 * English Wikipedia -> Article on Dew Point
 * Human reaction to high dew points
 * ---------------------------------
 *
 *      >24℃: Extremely uncomfortable, oppressive
 *  21 - 24℃: Very humid, quite uncomfortable
 *  18 - 21℃: Somewhat uncomfortable for most people at upper edge
 *  16 - 18℃: OK for most, but all perceive the humidity at upper edge
 *  13 - 16℃: Comfortable
 *  10 - 12℃: Very comfortable
 * <10℃     : A bit dry for some
 *
 * http://en.wikipedia.org/wiki/Dew_point
 * Last-Modified: 2009-03-24 (id=279476292)
 * Retrieved:     2009-03-29
 *
 * Copyright ©2009  Various Wikipedia editors
 * Licensed under the terms of the GNU Free Documentation License
 *
 * (The GFDL is incompatible with the GPL
 * - I consider this comment reproduction
 * to be "fair use".)
 */
#define MIN_DEWPC 0.0
#define MIN_DEWPC_WARN 6.0
#define MAX_DEWPC_WARN 18.0
#define MAX_DEWPC 21.0

int tray_init(struct th_data *data);
void tray_reset(HWND hWnd, struct th_data *data);
void tray_add(HWND hWnd, struct th_data *data);
void tray_update(HWND hWnd, struct th_data *data);
BOOL tray_activity(HWND hWnd, struct th_data *data, WPARAM wParam, LPARAM lParam);
void tray_remove(HWND hWnd, struct th_data *data);
