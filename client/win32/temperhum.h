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

#define TITLE "TEMPerHum Tray Icon\0"
#define DEFAULT_SERVICE "21576"

#define WM_APP_NET  (WM_APP+0)
#define WM_APP_TRAY (WM_APP+1)
#define WM_APP_SOCK (WM_APP+2)

#define NET_MSG_CONNECT 0

#define RETRY_TIMER_ID 1

enum conn_status {
	NOT_CONNECTED,
	CONNECTING,
	CONNECTED
};

struct tray_status {
	enum conn_status conn;
	char msg[512];
	double temperature_celsius;
	double relative_humidity;
	double dew_point;
};

struct th_data {
	HINSTANCE hInstance;
	int running;

	char *node;
	char *service;

#if HAVE_GETADDRINFO
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXSERV];
	struct addrinfo hints;
	struct addrinfo *addrs_res;
	struct addrinfo *addrs_cur;
#else
	struct sockaddr_in sa4;
	struct sockaddr_in6 sa6;
	int family;
	struct sockaddr *sa;
	int sa_len;
#endif
	SOCKET s;

	char parse_buf[128];
	unsigned int parse_pos;
	int def_sensor;

	NOTIFYICONDATA niData;
	int tray_ok;
	struct tray_status status;
};

void temperhum_shutdown(struct th_data *data, int status);
