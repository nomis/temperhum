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
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "debug.h"
#include "temperhum.h"
#include "tray.h"

void temperhum_run(char *ip, unsigned short port) {
	WSADATA wsaData;
	struct sockaddr_in sa4;
	struct sockaddr_in6 sa6;
	struct sockaddr *sa = NULL;
	int sa4_len = sizeof(sa4);
	int sa6_len = sizeof(sa6);
	int sa_len = 0;
	int family = AF_UNSPEC;
	int s = -1;
	long ret;
	struct tray_status status;
	char recv_buf[128];
	char parse_buf[128];
	unsigned int parse_pos = 0;

	odprintf("ip=%s", ip);
	odprintf("port=%d", port);

	ret = WSAStartup(MAKEWORD(2,0), &wsaData);
	if (ret != 0) {
		odprintf("WSAStartup: %d", ret);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Winsock2 startup failed (%d)");
		exit(EXIT_FAILURE);
	}

	/* TODO use getaddrinfo */
	ret = WSAStringToAddress(ip, AF_INET, NULL, (LPSOCKADDR)&sa4, &sa4_len);
	odprintf("WSAStringToAddress[IPv4]: %d", ret);
	if (ret == 0) {
		family = AF_INET;
		sa4.sin_family = AF_INET;
		sa4.sin_port = htons(port);

		sa = (struct sockaddr*)&sa4;
		sa_len = sa4_len;
	}

	ret = WSAStringToAddress(ip, AF_INET6, NULL, (LPSOCKADDR)&sa6, &sa6_len);
	odprintf("WSAStringToAddress[IPv6]: %d", ret);
	if (ret == 0) {
		family = AF_INET6;
		sa6.sin6_family = AF_INET6;
		sa6.sin6_port = htons(port);

		sa = (struct sockaddr*)&sa6;
		sa_len = sa6_len;
	}

	odprintf("family=%d", family);
	if (family == AF_UNSPEC) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Invalid IP \"%s\" specified", ip);
		exit(EXIT_FAILURE);
	}

	status.conn = NOT_CONNECTED;
	update_tray(&status);

	while (1) {
		if (status.conn != CONNECTED) {
			SetLastError(0);
			s = socket(family, SOCK_STREAM, IPPROTO_TCP);
			odprintf("socket: %d (%d)", s, GetLastError());

			if (s != -1) {
				int timeout = 5000; /* 5 seconds */

				SetLastError(0);
				ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout, sizeof(&timeout));
				odprintf("setsockopt: %d (%d)", ret, GetLastError());
				if (ret != 0) {
					closesocket(s);
					s = -1;
				}
			}

			if (s != -1) {
				status.conn = CONNECTING;
				update_tray(&status);

				SetLastError(0);
				ret = connect(s, sa, sa_len);
				odprintf("connect: %d (%d)", ret, GetLastError());
				if (ret == 0) {
					status.conn = CONNECTED;
					status.temperature_celsius = NAN;
					status.relative_humidity = NAN;
					status.dew_point = NAN;
					update_tray(&status);
				} else {
					status.conn = NOT_CONNECTED;
					update_tray(&status);

					closesocket(s);
					s = -1;
				}
			}

			if (s == -1)
				Sleep(1000);
		} else {
			SetLastError(0);
			ret = recv(s, recv_buf, sizeof(recv_buf), 0);
			odprintf("recv: %d (%d)", ret, GetLastError());
			if (ret <= 0) {
					status.conn = NOT_CONNECTED;
					update_tray(&status);

					parse_pos = 0;
					closesocket(s);
					s = -1;
			} else {
				int size, i;

				size = ret;
				for (i = 0; i < size; i++) {
					/* find a newline and parse the buffer */
					if (recv_buf[i] == '\n') {
						char msg_type[16];

						ret = sscanf(parse_buf, "%16s", msg_type);
						if (ret == 1) {
							odprintf("msg_type: %s", msg_type);
							if (!strcmp(msg_type, "TEMPC")) {
								ret = sscanf(parse_buf, "%*s %lf", &status.temperature_celsius);
								if (ret != 1)
									status.temperature_celsius = NAN;
							} else if (!strcmp(msg_type, "RHUM%")) {
								ret = sscanf(parse_buf, "%*s %lf", &status.relative_humidity);
								if (ret != 1)
									status.relative_humidity = NAN;
							} else if (!strcmp(msg_type, "DEWPC")) {
								ret = sscanf(parse_buf, "%*s %lf", &status.dew_point);
								if (ret != 1)
									status.dew_point = NAN;
							} else if (!strcmp(msg_type, "FLUSH")) {
								update_tray(&status);
							}
						}

						/* clear buffer */
						parse_pos = 0;

					/* buffer overflow */
					} else if (parse_pos == sizeof(parse_buf)/sizeof(char) - 1) {
						odprintf("parse: sender overflowed buffer waiting for '\\n'");
						parse_buf[0] = 0;
						parse_pos++;
					/* ignore */
					} else if (parse_pos > sizeof(parse_buf)/sizeof(char) - 1) {

					/* append to buffer */
					} else {
						parse_buf[parse_pos++] = recv_buf[i];
						parse_buf[parse_pos] = 0;
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	char *ip;
	unsigned short port = 21576;
	int i;

	odprintf("argc=%d", argc);
	for (i = 0; i < argc; i++)
		odprintf("argv[%d]=%s", i, argv[i]);

	if (argc < 2 || argc > 3) {
		odprintf("Usage: %s <ip> [port]", argv[0]);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Usage: %s <ip> [port]", argv[0]);
		exit(EXIT_FAILURE);
	}

	ip = argv[1];
	if (argc == 3)
		port = strtoul(argv[2], NULL, 10);

	temperhum_run(ip, port);
	exit(EXIT_SUCCESS);
}
