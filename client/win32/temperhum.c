#include <math.h>
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

	odprintf("ip=%s", ip);
	odprintf("port=%d", port);

	ret = WSAStartup(MAKEWORD(2,0), &wsaData);
	if (ret != 0) {
		odprintf("WSAStartup: %d", ret);
		MessageBox(NULL, "WSAStartup failed", TITLE, MB_OK|MB_ICONERROR);
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
		MessageBox(NULL, "Invalid IP specified", TITLE, MB_OK|MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	status.conn = NOT_CONNECTED;
	status.temperature_celsius = NAN;
	status.relative_humidity = NAN;
	status.dew_point = NAN;
	update_tray(&status);

	while (1) {
		if (status.conn != CONNECTED) {
			status.conn = NOT_CONNECTED;
			update_tray(&status);

			SetLastError(0);
			s = socket(family, SOCK_STREAM, IPPROTO_TCP);
			odprintf("socket: %d (%d)", s, GetLastError());

			if (s != -1) {
				int timeout = 5;

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
			
			exit(EXIT_SUCCESS);
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
		MessageBox(NULL, "Usage: temperhum.exe <ip> [port]", TITLE, MB_OK|MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	ip = argv[1];
	if (argc == 3)
		port = strtoul(argv[2], NULL, 10);

	temperhum_run(ip, port);
	exit(EXIT_SUCCESS);
}
