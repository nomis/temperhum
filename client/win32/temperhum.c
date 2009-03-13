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

#include "config.h"
#include "debug.h"
#include "temperhum.h"
#include "tray.h"

static int running = 1;

void temperhum_run(char *node, char *service) {
	WSADATA wsaData;
#if HAVE_GETADDRINFO
	struct addrinfo hints;
	struct addrinfo *addrs_res = NULL;
	struct addrinfo *addrs_cur = NULL;
#else
	struct sockaddr_in sa4;
	struct sockaddr_in6 sa6;
	int sa4_len = sizeof(sa4);
	int sa6_len = sizeof(sa6);
#endif
	int family = AF_UNSPEC;
	struct sockaddr *sa = NULL;
	int sa_len = 0;
	int s = -1;
	long ret;
	struct tray_status status;
	char recv_buf[128];
	char parse_buf[128];
	unsigned int parse_pos = 0;

	odprintf("node=%s", node);
	odprintf("service=%s", service);

	ret = WSAStartup(MAKEWORD(2,0), &wsaData);
	if (ret != 0) {
		odprintf("WSAStartup: %d", ret);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Winsock 2.0 startup failed (%d)");
		exit(EXIT_FAILURE);
	}

#if HAVE_GETADDRINFO
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	ret = getaddrinfo(node, service, &hints, &addrs_res);
	odprintf("getaddrinfo: %d", ret);
	if (ret != 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to resolve node \"%s\" service \"%s\"", node, service);
		exit(EXIT_FAILURE);
	}

	addrs_cur = addrs_res;
#else
	ret = WSAStringToAddress(node, AF_INET, NULL, (LPSOCKADDR)&sa4, &sa4_len);
	odprintf("WSAStringToAddress[IPv4]: %d", ret);
	if (ret == 0) {
		family = AF_INET;
		sa4.sin_family = AF_INET;
		sa4.sin_port = htons(strtoul(service, NULL, 10));

		sa = (struct sockaddr*)&sa4;
		sa_len = sa4_len;
	}

	ret = WSAStringToAddress(node, AF_INET6, NULL, (LPSOCKADDR)&sa6, &sa6_len);
	odprintf("WSAStringToAddress[IPv6]: %d", ret);
	if (ret == 0) {
		family = AF_INET6;
		sa6.sin6_family = AF_INET6;
		sa6.sin6_port = htons(strtoul(service, NULL, 10));

		sa = (struct sockaddr*)&sa6;
		sa_len = sa6_len;
	}

	odprintf("family=%d", family);
	if (family == AF_UNSPEC) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Invalid IP \"%s\" specified", node);
		exit(EXIT_FAILURE);
	}
#endif

	status.conn = NOT_CONNECTED;
	update_tray(&status);

	while (running) {
		if (status.conn != CONNECTED) {
#if HAVE_GETADDRINFO
			char hbuf[NI_MAXHOST];
			char sbuf[NI_MAXSERV];

			if (addrs_res == NULL || addrs_cur == NULL) {
				ret = getaddrinfo(node, service, &hints, &addrs_res);
				odprintf("getaddrinfo: %d", ret);
				if (ret != 0) {
					Sleep(5000);
					continue;
				}

				addrs_cur = addrs_res;
			}

			family = addrs_cur->ai_family;
			sa = addrs_cur->ai_addr;
			sa_len = addrs_cur->ai_addrlen;

			ret = getnameinfo(sa, sa_len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST|NI_NUMERICSERV);
			if (ret == 0) 
				odprintf("trying to connect to node \"%s\" service \"%s\"", hbuf, sbuf);
			addrs_cur = addrs_cur->ai_next;
#else
			odprintf("trying to connect to node \"%s\" service \"%s\"", node, service);
#endif

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

#if HAVE_GETADDR_INFO
					freeaddrinfo(addrs_res);
					addrs_res = NULL;
#endif
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

	if (s != -1)
		closesocket(s);

#if HAVE_GETADDRINFO
	if (addrs_res != NULL)
		freeaddrinfo(addrs_res);
#endif

	WSACleanup();
}

int main(int argc, char *argv[]) {
	char *node;
	char *service = "21576";
	int i;

	odprintf("argc=%d", argc);
	for (i = 0; i < argc; i++)
		odprintf("argv[%d]=%s", i, argv[i]);

	if (argc < 2 || argc > 3) {
#if HAVE_GETADDRINFO
		odprintf("Usage: %s <node (host/ip)> [service (port)]", argv[0]);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Usage: %s <node (host/ip)> [service (port)]", argv[0]);
#else
		odprintf("Usage: %s <ip> [port]", argv[0]);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Usage: %s <ip> [port]", argv[0]);
#endif
		exit(EXIT_FAILURE);
	}

	node = argv[1];
	if (argc == 3)
		service = argv[2];

	temperhum_run(node, service);
	exit(EXIT_SUCCESS);
}
