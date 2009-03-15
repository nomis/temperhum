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

int temperhum_run(HWND hwnd, char *node, char *service) {
	struct th_data data;
	int status;
	MSG msg;
	INT ret;
	LONG_PTR retlp;
	DWORD err;

	SetLastError(0);
	retlp = SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)&data);
	err = GetLastError();
	odprintf("SetWindowLongPtr: %p (%ld)", retlp, err);
	if (err != 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to set window pointer in GWL_USERDATA (%ld)", err);
		return EXIT_FAILURE;
	}

	data.node = node;
	data.service = service;

	tray_init(&data);
	tray_add(hwnd, &data);
	tray_update(hwnd, &data);

	ret = comms_init(&data);
	odprintf("comms_init: %d", ret);
	if (ret != 0) {
		data.running = 0;
		status = EXIT_FAILURE;
	} else {
		data.running = 1;
		status = EXIT_SUCCESS;

		SetLastError(0);
		ret = PostMessage(hwnd, WM_APP_NET, 0, NET_MSG_CONNECT);
		err = GetLastError();
		odprintf("PostMessage: %d (%ld)", ret, err);
		if (ret == 0) {
			data.running = 0;
			status = EXIT_FAILURE;
			mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to post initial connect message (%ld)", err);
		}
	}

	while (data.running) {
		SetLastError(0);
		ret = GetMessage(&msg, NULL, 0, 0);
#ifdef DEBUG
		err = GetLastError();
		odprintf("GetMessage: %d (%ld)", ret, err);
#endif

		/* Fatal error */
		if (ret == -1) {
#ifndef DEBUG
			err = GetLastError();
			odprintf("GetMessage: %d (%ld)", ret, err);
#endif
			break;
		}

		/* WM_QUIT */
		if (ret == 0) {
#ifndef DEBUG
			err = GetLastError();
			odprintf("GetMessage: %d (%ld)", ret, err);
#endif
			data.running = 0;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	comms_destroy(&data);
	tray_remove(hwnd, &data);

	SetLastError(0);
	retlp = SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)NULL);
	err = GetLastError();
	odprintf("SetWindowLongPtr: %p (%ld)", retlp, err);

	return status;
}

LRESULT CALLBACK temperhum_window(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	struct th_data *data;
	BOOL retb;
	INT ret;
	DWORD err;

	SetLastError(0);
	data = (struct th_data*)GetWindowLongPtr(hwnd, GWL_USERDATA);
	err = GetLastError();

	odprintf("temperhum_window: hwnd=%p (data=%p) msg=%u wparam=%d lparam=%d", hwnd, data, uMsg, wParam, lParam);
	if (data == NULL) {
		odprintf("GetWindowLongPtr: %p (%ld)", data, err);

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_APP_NET:
		switch (lParam) {
		case NET_MSG_CONNECT:
			ret = comms_connect(hwnd, data);
			if (ret != 0) {
				// TODO retry in 5s
			}
			return TRUE;
		}
		break;

	case WM_APP_TRAY:
		retb = tray_activity(hwnd, data, wParam, lParam);
		if (retb == TRUE)
			return TRUE;
		break;

	case WM_APP_SOCK:
		ret = comms_activity(hwnd, data, (SOCKET)wParam, WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam));
		if (ret != 0) {
			// TODO retry in 5s
		}
		return TRUE;
	}

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
	char service[512] = DEFAULT_SERVICE;
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
	hwnd = CreateWindowEx(0, wcx.lpszClassName, TITLE, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hinst, NULL);
	err = GetLastError();
	odprintf("CreateWindowEx: %p (%ld)", hwnd, err);
	if (hwnd == NULL) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Failed to create window (%ld)", err);
		status = EXIT_FAILURE;
		goto unregister_class;
	}

	SetLastError(0);
	ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	err = GetLastError();
	odprintf("WSAStartup: %d (%ld)", ret, err);
	if (ret != 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Winsock 2.2 startup failed (%d)", ret);
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
