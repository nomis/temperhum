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

int temperhum_run(HWND hwnd, char *node, char *service) {
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
	int ret;
	DWORD err;
	struct tray_status status;
	char recv_buf[128];
	char parse_buf[128];
	unsigned int parse_pos = 0;
	int def_sensor = 0;

	odprintf("node=%s", node);
	odprintf("service=%s", service);

	tray_add(hwnd);

	status.conn = NOT_CONNECTED;
	tray_update(hwnd, &status);

#if HAVE_GETADDRINFO
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	SetLastError(0);
	ret = getaddrinfo(node, service, &hints, &addrs_res);
	err = GetLastError();
	odprintf("getaddrinfo: %d (%d)", ret, err);
	if (ret != 0) {
		ret = snprintf(status.error, sizeof(status.error), "Unable to resolve node \"%s\" service \"%s\" (%d)", node, service, ret);
		if (ret < 0)
			status.error[0] = 0;
	}

	if (addrs_res == NULL) {
		odprintf("no results");
		ret = snprintf(status.error, sizeof(status.error), "No results resolving node \"%s\" service \"%s\"", node, service);
		if (ret < 0)
			status.error[0] = 0;
	}

	addrs_cur = addrs_res;
#else
	SetLastError(0);
	ret = WSAStringToAddress(node, AF_INET, NULL, (LPSOCKADDR)&sa4, &sa4_len);
	err = GetLastError();
	odprintf("WSAStringToAddress[IPv4]: %d (%ld)", ret, err);
	if (ret == 0) {
		family = AF_INET;
		sa4.sin_family = AF_INET;
		sa4.sin_port = htons(strtoul(service, NULL, 10));

		sa = (struct sockaddr*)&sa4;
		sa_len = sa4_len;
	}

	SetLastError(0);
	ret = WSAStringToAddress(node, AF_INET6, NULL, (LPSOCKADDR)&sa6, &sa6_len);
	err = GetLastError();
	odprintf("WSAStringToAddress[IPv6]: %d (%ld)", ret, err);
	if (ret == 0) {
		family = AF_INET6;
		sa6.sin6_family = AF_INET6;
		sa6.sin6_port = htons(strtoul(service, NULL, 10));

		sa = (struct sockaddr*)&sa6;
		sa_len = sa6_len;
	}

	odprintf("family=%d", family);
	if (family == AF_UNSPEC) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to connect: Invalid IP \"%s\"", node);
		return EXIT_FAILURE;
	}
#endif

	while (running) {
		if (status.conn != CONNECTED) {
#if HAVE_GETADDRINFO
			char hbuf[NI_MAXHOST];
			char sbuf[NI_MAXSERV];

			if (addrs_cur == NULL && addrs_res != NULL) {
				freeaddrinfo(addrs_res);
				addrs_res = NULL;
			}

			if (addrs_res == NULL) {
				SetLastError(0);
				ret = getaddrinfo(node, service, &hints, &addrs_res);
				err = GetLastError();
				odprintf("getaddrinfo: %d (%ld)", ret, err);
				if (ret != 0) {
					ret = snprintf(status.error, sizeof(status.error), "Unable to resolve node \"%s\" service \"%s\" (%d)", node, service, ret);
					if (ret < 0)
						status.error[0] = 0;
					tray_update(hwnd, &status);

					Sleep(5000);
					continue;
				}

				if (addrs_res == NULL) {
					odprintf("no results");
					ret = snprintf(status.error, sizeof(status.error), "No results resolving node \"%s\" service \"%s\"", node, service);
					if (ret < 0)
						status.error[0] = 0;
					tray_update(hwnd, &status);

					Sleep(5000);
					continue;
				}

				addrs_cur = addrs_res;
			}

			family = addrs_cur->ai_family;
			sa = addrs_cur->ai_addr;
			sa_len = addrs_cur->ai_addrlen;

			SetLastError(0);
			ret = getnameinfo(sa, sa_len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST|NI_NUMERICSERV);
			err = GetLastError();
			odprintf("getnameinfo: %d (%ld)", ret, err);
			if (ret == 0) {
				odprintf("trying to connect to node \"%s\" service \"%s\"", hbuf, sbuf);
			} else {
				hbuf[0] = 0;
				sbuf[0] = 0;
			}
			addrs_cur = addrs_cur->ai_next;
#else
			odprintf("trying to connect to node \"%s\" service \"%s\"", node, service);
#endif

			SetLastError(0);
			s = socket(family, SOCK_STREAM, IPPROTO_TCP);
			err = GetLastError();
			odprintf("socket: %d (%ld)", s, err);

			if (s == -1) {
				ret = snprintf(status.error, sizeof(status.error), "Unable to create socket (%ld)", err);
				if (ret < 0)
					status.error[0] = 0;
				tray_update(hwnd, &status);
			}

			if (s != -1) {
				int timeout = 5000; /* 5 seconds */

				SetLastError(0);
				ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout, sizeof(&timeout));
				err = GetLastError();
				odprintf("setsockopt: %d (%ld)", ret, err);
				if (ret != 0) {
					ret = snprintf(status.error, sizeof(status.error), "Unable to set socket timeout (%ld)", err);
					if (ret < 0)
						status.error[0] = 0;
					tray_update(hwnd, &status);

					SetLastError(0);
					ret = closesocket(s);
					err = GetLastError();
					odprintf("closesocket: %d (%ld)", ret, err);
					s = -1;
				}
			}

			if (s != -1) {
				status.conn = CONNECTING;
				tray_update(hwnd, &status);

				SetLastError(0);
				ret = connect(s, sa, sa_len);
				err = GetLastError();
				odprintf("connect: %d (%ld)", ret, err);
				if (ret == 0) {
					status.conn = CONNECTED;
					status.error[0] = 0;
					status.temperature_celsius = NAN;
					status.relative_humidity = NAN;
					status.dew_point = NAN;
					tray_update(hwnd, &status);

#if HAVE_GETADDRINFO
					freeaddrinfo(addrs_res);
					addrs_res = NULL;
#endif
					parse_pos = 0;
				} else {
					status.conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
					if (hbuf[0] != 0 && sbuf[0] != 0) {
						ret = snprintf(status.error, sizeof(status.error), "Error connecting to node \"%s\" service \"%s\" (%ld)", hbuf, sbuf, err);
					} else {
#endif
						ret = snprintf(status.error, sizeof(status.error), "Error connecting to node \"%s\" service \"%s\" (%ld)", node, service, err);
#if HAVE_GETADDRINFO
					}
#endif
					if (ret < 0)
						status.error[0] = 0;
					tray_update(hwnd, &status);

					SetLastError(0);
					ret = closesocket(s);
					err = GetLastError();
					odprintf("closesocket: %d (%ld)", ret, err);
					s = -1;
				}
			}

			if (s == -1)
				Sleep(1000);
		} else {
			SetLastError(0);
			ret = recv(s, recv_buf, sizeof(recv_buf), 0);
			err = GetLastError();
			odprintf("recv: %d (%ld)", ret, err);
			if (ret <= 0) {
					status.conn = NOT_CONNECTED;
					ret = snprintf(status.error, sizeof(status.error), "Error reading from server (%ld)", err);
					if (ret < 0)
						status.error[0] = 0;
					tray_update(hwnd, &status);

					SetLastError(0);
					ret = closesocket(s);
					err = GetLastError();
					odprintf("closesocket: %d (%ld)", ret, err);
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
							if (!strcmp(msg_type, "SENSD")) {
								def_sensor = 1;
							} else if (!strcmp(msg_type, "SENSR")) {
								def_sensor = 0;
							} else if (def_sensor) {
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
								} else if (!strcmp(msg_type, "SENSF")) {
									tray_update(hwnd, &status);
								}
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

	if (s != -1) {
		SetLastError(0);
		ret = closesocket(s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
	}

#if HAVE_GETADDRINFO
	if (addrs_res != NULL)
		freeaddrinfo(addrs_res);
#endif

	tray_remove();
	return EXIT_SUCCESS;
}

LRESULT CALLBACK temperhum_window(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	odprintf("temperhum_window: hwnd=%p msg=%u wparam=%08x lparam=%08x", hwnd, uMsg, wParam, lParam);

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpCmdLine, int nShowCmd) {
	BOOL retb;
	HLOCAL retp;
	ATOM cls;
	HWND hwnd;
	DWORD err;
	WNDCLASSEX wcx;
	WSADATA wsaData;
	LPWSTR *argv;
	int argc;
	char node[512];
	char service[512] = "21576";
	char buf[512];
	int ret, status, i;

	SetLastError(0);
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	err = GetLastError();
	odprintf("CommandLineToArgvW: %p (%ld)", argv, err);
	if (argv == NULL) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Error getting command line arguments (%ld)", err);
		status = EXIT_FAILURE;
		goto done;
	}

	odprintf("argc=%d", argc);
	for (i = 0; i < argc; i++) {
		ret = snprintf(buf, sizeof(buf), "%S", argv[i]);
		if (ret < 0)
			buf[0] = 0;
		odprintf("argv[%d]=%s", i, buf);
	}

	if (argc < 2 || argc > 3) {
#if HAVE_GETADDRINFO
		odprintf("Usage: %s <node (host/ip)> [service (port)]", argv[0]);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Usage: %s <node (host/ip)> [service (port)]", argv[0]);
#else
		odprintf("Usage: %s <ip> [port]", argv[0]);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Usage: %s <ip> [port]", argv[0]);
#endif
		status = EXIT_FAILURE;
		goto free_argv;
	}

	ret = snprintf(node, sizeof(node), "%S", argv[1]);
	if (ret < 0)
		node[0] = 0;

	if (argc == 3) {
		ret = snprintf(service, sizeof(service), "%S", argv[2]);
		if (ret < 0)
			service[0] = 0;
	}

	wcx.cbSize = sizeof(wcx);
	wcx.style = 0;
	wcx.lpfnWndProc = temperhum_window;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hinst;
	wcx.hIcon = NULL;
	wcx.hCursor = NULL;
	wcx.hbrBackground = NULL;
	wcx.lpszMenuName = NULL;
	wcx.lpszClassName = "temperhum";
	wcx.hIconSm = NULL;

	SetLastError(0);
	cls = RegisterClassEx(&wcx);
	err = GetLastError();
	odprintf("RegisterClassEx: %d (%ld)", cls, err);
	if (cls == 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Failed to register class (%ld)", err);
		status = EXIT_FAILURE;
		goto free_argv;
	}

	SetLastError(0);
	hwnd = CreateWindow(wcx.lpszClassName, TITLE, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hinst, NULL);
	err = GetLastError();
	odprintf("CreateWindow: %p (%ld)", hwnd, err);
	if (hwnd == NULL) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Failed to create window (%ld)", err);
		status = EXIT_FAILURE;
		goto unregister_class;
	}

	SetLastError(0);
	ret = WSAStartup(MAKEWORD(2,0), &wsaData);
	err = GetLastError();
	odprintf("WSAStartup: %d (%ld)", ret, err);
	if (ret != 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Winsock 2.0 startup failed (%d)", ret);
		status = EXIT_FAILURE;
		goto destroy_window;
	}

	status = temperhum_run(hwnd, node, service);

	SetLastError(0);
	ret = WSACleanup();
	err = GetLastError();
	odprintf("WSACleanup: %d (%ld)", ret, err);

destroy_window:
	SetLastError(0);
	retb = DestroyWindow(hwnd);
	err = GetLastError();
	odprintf("DestroyWindow: %d (%ld)", retb, err);

unregister_class:
	SetLastError(0);
	retb = UnregisterClass(wcx.lpszClassName, hinst);
	err = GetLastError();
	odprintf("UnregisterClass: %d (%ld)", retb, err);

free_argv:
	SetLastError(0);
	retp = LocalFree(argv);
	err = GetLastError();
	odprintf("LocalFree: %p (%ld)", ret, err);

done:
	exit(status);
}
