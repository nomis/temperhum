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
#include "comms.h"
#include "tray.h"

int comms_init(struct th_data *data) {
	INT ret;
	DWORD err;
#if HAVE_GETADDRINFO

	odprintf("comms[init]: node=%s service=%s", data->node, data->service);

	data->hbuf[0] = 0;
	data->sbuf[0] = 0;
	data->addrs_res = NULL;
	data->addrs_cur = NULL;

	data->hints.ai_flags = 0;
	data->hints.ai_family = AF_UNSPEC;
	data->hints.ai_socktype = SOCK_STREAM;
	data->hints.ai_protocol = IPPROTO_TCP;
	data->hints.ai_addrlen = 0;
	data->hints.ai_addr = NULL;
	data->hints.ai_canonname = NULL;
	data->hints.ai_next = NULL;

	SetLastError(0);
	ret = getaddrinfo(data->node, data->service, &data->hints, &data->addrs_res);
	err = GetLastError();
	odprintf("getaddrinfo: %d (%d)", ret, err);
	if (ret != 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to resolve node \"%s\" service \"%s\" (%d)", data->node, data->service, ret);
		return 1;
	}

	if (data->addrs_res == NULL) {
		odprintf("no results");
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "No results resolving node \"%s\" service \"%s\"", data->node, data->service);
		return 1;
	}

	data->addrs_cur = data->addrs_res;
#else
	int sa4_len = sizeof(data->sa4);
	int sa6_len = sizeof(data->sa6);

	odprintf("comms[init]: node=%s service=%s", data->node, data->service);

	data->family = AF_UNSPEC;
	data->sa = NULL;
	data->sa_len = 0;

	SetLastError(0);
	ret = WSAStringToAddress(data->node, AF_INET, NULL, (LPSOCKADDR)&data->sa4, &sa4_len);
	err = GetLastError();
	odprintf("WSAStringToAddress[IPv4]: %d (%ld)", ret, err);
	if (ret == 0) {
		data->family = AF_INET;
		data->sa4.sin_family = AF_INET;
		data->sa4.sin_port = htons(strtoul(data->service, NULL, 10));

		data->sa = (struct sockaddr*)&data->sa4;
		data->sa_len = sa4_len;
	}

	SetLastError(0);
	ret = WSAStringToAddress(data->node, AF_INET6, NULL, (LPSOCKADDR)&data->sa6, &sa6_len);
	err = GetLastError();
	odprintf("WSAStringToAddress[IPv6]: %d (%ld)", ret, err);
	if (ret == 0) {
		data->family = AF_INET6;
		data->sa6.sin6_family = AF_INET6;
		data->sa6.sin6_port = htons(strtoul(data->service, NULL, 10));

		data->sa = (struct sockaddr*)&data->sa6;
		data->sa_len = sa6_len;
	}

	odprintf("family=%d", data->family);
	if (family == AF_UNSPEC) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to connect: Invalid IP \"%s\"", data->node);
		return 1;
	}
#endif

	data->s = INVALID_SOCKET;
	return 0;
}

void comms_destroy(struct th_data *data) {
	odprintf("comms[destroy]");

#if HAVE_GETADDRINFO
	if (data->addrs_res != NULL) {
		freeaddrinfo(data->addrs_res);
		data->addrs_res = NULL;
	}
#endif

	comms_disconnect(data);
}

void comms_disconnect(struct th_data *data) {
	INT ret;
	DWORD err;

	odprintf("comms[disconnect]");

	data->status.conn = NOT_CONNECTED;

	if (data->s != INVALID_SOCKET) {
		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);

		data->s = INVALID_SOCKET;
	}
}

int comms_connect(HWND hwnd, struct th_data *data) {
	struct tray_status *status = &data->status;
	int timeout = 5000; /* 5 seconds */
	INT ret;
	DWORD err;

	odprintf("comms[connect]");

	if (!data->running)
		return 0;

	if (data->s != INVALID_SOCKET)
		return 0;

	status->conn = NOT_CONNECTED;
	tray_update(hwnd, data);

#if HAVE_GETADDRINFO
	if (data->addrs_cur == NULL && data->addrs_res != NULL) {
		freeaddrinfo(data->addrs_res);
		data->addrs_res = NULL;
	}

	if (data->addrs_res == NULL) {
		SetLastError(0);
		ret = getaddrinfo(data->node, data->service, &data->hints, &data->addrs_res);
		err = GetLastError();
		odprintf("getaddrinfo: %d (%ld)", ret, err);
		if (ret != 0) {
			ret = snprintf(status->error, sizeof(status->error), "Unable to resolve node \"%s\" service \"%s\" (%d)", data->node, data->service, ret);
			if (ret < 0)
				status->error[0] = 0;
			tray_update(hwnd, data);

			return 1;
		}

		if (data->addrs_res == NULL) {
			odprintf("no results");
			ret = snprintf(status->error, sizeof(status->error), "No results resolving node \"%s\" service \"%s\"", data->node, data->service);
			if (ret < 0)
				status->error[0] = 0;
			tray_update(hwnd, data);

			return 1;
		}

		data->addrs_cur = data->addrs_res;
	}

	SetLastError(0);
	ret = getnameinfo(data->addrs_cur->ai_addr, data->addrs_cur->ai_addrlen, data->hbuf, sizeof(data->hbuf), data->sbuf, sizeof(data->sbuf), NI_NUMERICHOST|NI_NUMERICSERV);
	err = GetLastError();
	odprintf("getnameinfo: %d (%ld)", ret, err);
	if (ret == 0) {
		odprintf("trying to connect to node \"%s\" service \"%s\"", data->hbuf, data->sbuf);
	} else {
		data->hbuf[0] = 0;
		data->sbuf[0] = 0;
	}
#else
	odprintf("trying to connect to node \"%s\" service \"%s\"", node, service);
#endif

	SetLastError(0);
#if HAVE_GETADDRINFO
	data->s = socket(data->addrs_cur->ai_family, SOCK_STREAM, IPPROTO_TCP);
#else
	data->s = socket(data->family, SOCK_STREAM, IPPROTO_TCP);
#endif
	err = GetLastError();
	odprintf("socket: %d (%ld)", data->s, err);

	if (data->s == INVALID_SOCKET) {
		ret = snprintf(status->error, sizeof(status->error), "Unable to create socket (%ld)", err);
		if (ret < 0)
			status->error[0] = 0;
		tray_update(hwnd, data);

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}

	SetLastError(0);
	ret = setsockopt(data->s, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout, sizeof(&timeout));
	err = GetLastError();
	odprintf("setsockopt: %d (%ld)", ret, err);
	if (ret != 0) {
		ret = snprintf(status->error, sizeof(status->error), "Unable to set socket timeout (%ld)", err);
		if (ret < 0)
			status->error[0] = 0;
		tray_update(hwnd, data);

		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
		data->s = INVALID_SOCKET;

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}

	SetLastError(0);
	ret = WSAAsyncSelect(data->s, hwnd, WM_APP_SOCK, FD_CONNECT|FD_READ|FD_CLOSE);
	err = GetLastError();
	odprintf("WSAAsyncSelect: %d (%ld)", ret, err);
	if (ret != 0) {
		ret = snprintf(status->error, sizeof(status->error), "Unable to async select on socket (%ld)", err);
		if (ret < 0)
			status->error[0] = 0;
		tray_update(hwnd, data);

		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
		data->s = INVALID_SOCKET;

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}

	status->conn = CONNECTING;
#if HAVE_GETADDRINFO
	if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
		ret = snprintf(status->error, sizeof(status->error), "node \"%s\" service \"%s\" (%ld)", data->hbuf, data->sbuf, err);
	} else {
#endif
		ret = snprintf(status->error, sizeof(status->error), "node \"%s\" service \"%s\" (%ld)", data->node, data->service, err);
#if HAVE_GETADDRINFO
	}
#endif
	if (ret < 0)
		status->error[0] = 0;
	tray_update(hwnd, data);

	SetLastError(0);
#if HAVE_GETADDRINFO
	ret = connect(data->s, data->addrs_cur->ai_addr, data->addrs_cur->ai_addrlen);
#else
	ret = connect(data->s, data->sa, data->sa_len);
#endif
	err = GetLastError();
	odprintf("connect: %d (%ld)", ret, err);
	if (ret == 0 || err == WSAEWOULDBLOCK) {
		return 0;
	} else {
		status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
		if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
			ret = snprintf(status->error, sizeof(status->error), "Error connecting to node \"%s\" service \"%s\"", data->hbuf, data->sbuf);
		} else {
#endif
			ret = snprintf(status->error, sizeof(status->error), "Error connecting to node \"%s\" service \"%s\"", data->node, data->service);
#if HAVE_GETADDRINFO
		}
#endif
		if (ret < 0)
			status->error[0] = 0;
		tray_update(hwnd, data);

		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
		data->s = INVALID_SOCKET;

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}
}

int comms_activity(HWND hwnd, struct th_data *data, SOCKET s, WORD sEvent, WORD sError) {
	struct tray_status *status = &data->status;
	INT ret;
	DWORD err;

	odprintf("comms[activity]: s=%p sEvent=%d sError=%d", s, sEvent, sError);

	if (!data->running)
		return 0;

	if (data->s != s)
		return 0;

	switch (sEvent) {
	case FD_CONNECT:
		odprintf("FD_CONNECT %s", status->conn == CONNECTING ? "OK" : "?");
		if (status->conn != CONNECTING)
			return 0;

		if (sError == 0) {
			status->conn = CONNECTED;
			status->error[0] = 0;
			status->temperature_celsius = NAN;
			status->relative_humidity = NAN;
			status->dew_point = NAN;
			tray_update(hwnd, data);

#if HAVE_GETADDRINFO
			freeaddrinfo(data->addrs_res);
			data->addrs_res = NULL;
#endif
			data->parse_pos = 0;
			data->def_sensor = 0;

			// TODO: read timeout
			return 0;
		} else {
			status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
			if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
				ret = snprintf(status->error, sizeof(status->error), "Error connecting to node \"%s\" service \"%s\" (%d)", data->hbuf, data->sbuf, sError);
			} else {
#endif
				ret = snprintf(status->error, sizeof(status->error), "Error connecting to node \"%s\" service \"%s\" (%d)", data->node, data->service, sError);
#if HAVE_GETADDRINFO
			}
#endif
			if (ret < 0)
				status->error[0] = 0;
			tray_update(hwnd, data);

			SetLastError(0);
			ret = closesocket(data->s);
			err = GetLastError();
			odprintf("closesocket: %d (%ld)", ret, err);

			data->s = INVALID_SOCKET;
			return 1;
		}

	case FD_READ:
		odprintf("FD_READ %s", status->conn == CONNECTED ? "OK" : "?");
		if (status->conn != CONNECTED)
			return 0;

		if (sError == 0) {
			char recv_buf[128];

			SetLastError(0);
			ret = recv(data->s, recv_buf, sizeof(recv_buf), 0);
			err = GetLastError();
			odprintf("recv: %d (%ld)", ret, err);
			if (ret <= 0) {
				status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
				if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
					ret = snprintf(status->error, sizeof(status->error), "Error reading from node \"%s\" service \"%s\" (%ld)", data->hbuf, data->sbuf, err);
				} else {
#endif
					ret = snprintf(status->error, sizeof(status->error), "Error reading from node \"%s\" service \"%s\" (%ld)", data->node, data->service, err);
#if HAVE_GETADDRINFO
				}
#endif
				if (ret < 0)
					status->error[0] = 0;
				tray_update(hwnd, data);

				SetLastError(0);
				ret = closesocket(data->s);
				err = GetLastError();
				odprintf("closesocket: %d (%ld)", ret, err);

				data->s = INVALID_SOCKET;
				return 1;
			} else if (err == WSAEWOULDBLOCK) {
				return 0;
			} else {
				int size, i;

				size = ret;
				for (i = 0; i < size; i++) {
					/* find a newline and parse the buffer */
					if (recv_buf[i] == '\n') {
						comms_parse(hwnd, data);

						/* clear buffer */
						data->parse_pos = 0;

					/* buffer overflow */
					} else if (data->parse_pos == sizeof(data->parse_buf)/sizeof(char) - 1) {
						odprintf("parse: sender overflowed buffer waiting for '\\n'");
						data->parse_buf[0] = 0;
						data->parse_pos++;

					/* ignore */
					} else if (data->parse_pos > sizeof(data->parse_buf)/sizeof(char) - 1) {

					/* append to buffer */
					} else {
						data->parse_buf[data->parse_pos++] = recv_buf[i];
						data->parse_buf[data->parse_pos] = 0;
					}
				}

				// TODO: read timeout
				return 0;
			}
		} else {
			status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
			if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
				ret = snprintf(status->error, sizeof(status->error), "Error reading from node \"%s\" service \"%s\" (%d)", data->hbuf, data->sbuf, sError);
			} else {
#endif
				ret = snprintf(status->error, sizeof(status->error), "Error reading from node \"%s\" service \"%s\" (%d)", data->node, data->service, sError);
#if HAVE_GETADDRINFO
			}
#endif
			if (ret < 0)
				status->error[0] = 0;
			tray_update(hwnd, data);

			SetLastError(0);
			ret = closesocket(data->s);
			err = GetLastError();
			odprintf("closesocket: %d (%ld)", ret, err);

			data->s = INVALID_SOCKET;
			return 1;
		}

	case FD_CLOSE:
		odprintf("FD_CLOSE %s", status->conn == CONNECTED ? "OK" : "?");
		if (status->conn != CONNECTED)
			return 0;

		status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
		if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
			ret = snprintf(status->error, sizeof(status->error), "Lost connection to node \"%s\" service \"%s\" (%d)", data->hbuf, data->sbuf, sError);
		} else {
#endif
			ret = snprintf(status->error, sizeof(status->error), "Lost connection to node \"%s\" service \"%s\" (%d)", data->node, data->service, sError);
#if HAVE_GETADDRINFO
		}
#endif
		if (ret < 0)
			status->error[0] = 0;
		tray_update(hwnd, data);

		data->s = INVALID_SOCKET;
		return 0;

	default:
		return 0;
	}
}

void comms_parse(HWND hwnd, struct th_data *data) {
	struct tray_status *status = &data->status;
	char msg_type[16];

	if (sscanf(data->parse_buf, "%16s", msg_type) == 1) {
		odprintf("msg_type: %s", msg_type);
		if (!strcmp(msg_type, "SENSD")) {
			data->def_sensor = 1;
		} else if (!strcmp(msg_type, "SENSR")) {
			data->def_sensor = 0;
		} else if (data->def_sensor) {
			if (!strcmp(msg_type, "TEMPC")) {
				if (sscanf(data->parse_buf, "%*s %lf", &status->temperature_celsius) != 1)
					status->temperature_celsius = NAN;
			} else if (!strcmp(msg_type, "RHUM%")) {
				if (sscanf(data->parse_buf, "%*s %lf", &status->relative_humidity) != 1)
					status->relative_humidity = NAN;
			} else if (!strcmp(msg_type, "DEWPC")) {
				if (sscanf(data->parse_buf, "%*s %lf", &status->dew_point) != 1)
					status->dew_point = NAN;
			} else if (!strcmp(msg_type, "SENSF")) {
				tray_update(hwnd, data);
			}
		}
	}
}
