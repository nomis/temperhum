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
#include "icon.h"
#include "tray.h"

int temperhum_run(HINSTANCE hInstance, HWND hWnd, char *node, char *service) {
	struct th_data data;
	int status;
	MSG msg;
	INT ret;
	LONG_PTR retlp;
	DWORD err;

	odprintf("temperhum[run]");

	SetLastError(0);
	retlp = SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)&data);
	err = GetLastError();
	odprintf("SetWindowLongPtr: %p (%ld)", retlp, err);
	if (err != 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to set window pointer in GWL_USERDATA (%ld)", err);
		return EXIT_FAILURE;
	}

	data.hInstance = hInstance;
	data.node = node;
	data.service = service;

	data.running = 0;
	status = EXIT_FAILURE;

	ret = icon_init();
	odprintf("icon_init: %d", ret);
	if (ret != 0)
		goto fail_icon;

	ret = tray_init(&data);
	odprintf("tray_init: %d", ret);
	if (ret != 0)
		goto fail_tray;

	tray_add(hWnd, &data);
	tray_update(hWnd, &data);

	ret = comms_init(&data);
	odprintf("comms_init: %d", ret);
	if (ret != 0)
		goto fail_comms;

	SetLastError(0);
	ret = PostMessage(hWnd, WM_APP_NET, 0, NET_MSG_CONNECT);
	err = GetLastError();
	odprintf("PostMessage: %d (%ld)", ret, err);
	if (ret == 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to post initial connect message (%ld)", err);
		goto fail_connect;
	}

	data.running = 1;
	status = EXIT_SUCCESS;

	while (data.running) {
		SetLastError(0);
		ret = GetMessage(&msg, NULL, 0, 0);
#if DEBUG >= 2
		err = GetLastError();
		odprintf("GetMessage: %d (%ld)", ret, err);
#endif

		/* Fatal error */
		if (ret == -1) {
#if DEBUG < 2
			err = GetLastError();
			odprintf("GetMessage: %d (%ld)", ret, err);
#endif
			break;
		}

		/* WM_QUIT */
		if (ret == 0) {
#if DEBUG < 2
			err = GetLastError();
			odprintf("GetMessage: %d (%ld)", ret, err);
#endif
			data.running = 0;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

fail_connect:
	comms_destroy(&data);

fail_comms:
	tray_remove(hWnd, &data);

fail_tray:
	icon_free();

fail_icon:
	SetLastError(0);
	retlp = SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)NULL);
	err = GetLastError();
	odprintf("SetWindowLongPtr: %p (%ld)", retlp, err);

	return status;
}

void temperhum_shutdown(struct th_data *data, int status) {
	odprintf("temperhum[shutdown]");

	comms_disconnect(data);
	data->running = 0;

	PostQuitMessage(status);
}

void temperhum_retry(HWND hWnd, struct th_data *data) {
	INT ret;
	DWORD err;

	odprintf("temperhum[retry]");

	if (data->running) {
		SetLastError(0);
		ret = SetTimer(hWnd, RETRY_TIMER_ID, 5000, NULL); /* 5 seconds */
		err = GetLastError();
		odprintf("SetTimer: %d (%ld)", data, err);
		if (ret == 0) {
			mbprintf(TITLE, MB_OK|MB_ICONERROR, "Error starting connection retry timer (%ld)", err);
			temperhum_shutdown(data, EXIT_FAILURE);
		}
	}
}

LRESULT CALLBACK temperhum_window(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	struct th_data *data;
	BOOL retb;
	INT ret;
	DWORD err;

	SetLastError(0);
	data = (struct th_data*)GetWindowLongPtr(hWnd, GWL_USERDATA);
	err = GetLastError();

	odprintf("temperhum[window]: hWnd=%p (data=%p) msg=%u wparam=%d lparam=%d", hWnd, data, uMsg, wParam, lParam);
	if (data == NULL) {
		odprintf("GetWindowLongPtr: %p (%ld)", data, err);

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_APP_NET:
		switch (lParam) {
		case NET_MSG_CONNECT:
			ret = comms_connect(hWnd, data);
			if (ret != 0)
				temperhum_retry(hWnd, data);
			return TRUE;
		}
		break;

	case WM_APP_TRAY:
		retb = tray_activity(hWnd, data, wParam, lParam);
		if (retb == TRUE)
			return TRUE;
		break;

	case WM_APP_SOCK:
		ret = comms_activity(hWnd, data, (SOCKET)wParam, WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam));
		if (ret != 0)
			temperhum_retry(hWnd, data);
		return TRUE;

	case WM_TIMER:
		switch (wParam) {
		case RETRY_TIMER_ID:
			ret = comms_connect(hWnd, data);
			if (ret != 0)
				temperhum_retry(hWnd, data);
			return TRUE;
		}
		break;

	default:
		if (uMsg == data->taskbarCreated)
			tray_reset(hWnd, data);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstancePrev, LPSTR lpCmdLine, int nShowCmd) {
	BOOL retb;
	HLOCAL retp;
	ATOM cls;
	HWND hWnd;
	DWORD err;
	WNDCLASSEX wcx;
	WSADATA wsaData;
	LPWSTR *argv;
	int argc;
	char node[512];
	char service[512] = DEFAULT_SERVICE;
	int ret, status, i;
	(void)hInstancePrev;
	(void)lpCmdLine;
	(void)nShowCmd;

	odprintf("temperhum[main]: _WIN32_WINNT=%04x _WIN32_IE=%04x", _WIN32_WINNT, _WIN32_IE);

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
	for (i = 0; i < argc; i++)
		odprintf("argv[%d]=%S", i, argv[i]);

	if (argc < 2 || argc > 3) {
#if HAVE_GETADDRINFO
		odprintf("Usage: %S <node (host/ip)> [service (port)]", argv[0]);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Usage: %S <node (host/ip)> [service (port)]", argv[0]);
#else
		odprintf("Usage: %S <ip> [port]", argv[0]);
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Usage: %S <ip> [port]", argv[0]);
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
	wcx.hInstance = hInstance;
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

	/* Can't use HWND_MESSAGE... because it won't receive broadcasts like TaskbarCreated */
	SetLastError(0);
	hWnd = CreateWindowEx(0, wcx.lpszClassName, TITLE, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, /*HWND_MESSAGE*/ NULL, NULL, hInstance, NULL);
	err = GetLastError();
	odprintf("CreateWindowEx: %p (%ld)", hWnd, err);
	if (hWnd == NULL) {
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

	status = temperhum_run(hInstance, hWnd, node, service);

	SetLastError(0);
	ret = WSACleanup();
	err = GetLastError();
	odprintf("WSACleanup: %d (%ld)", ret, err);

destroy_window:
	SetLastError(0);
	retb = DestroyWindow(hWnd);
	err = GetLastError();
	odprintf("DestroyWindow: %s (%ld)", retb == TRUE ? "TRUE" : "FALSE", err);

unregister_class:
	SetLastError(0);
	retb = UnregisterClass(wcx.lpszClassName, hInstance);
	err = GetLastError();
	odprintf("UnregisterClass: %s (%ld)", retb == TRUE ? "TRUE" : "FALSE", err);

free_argv:
	SetLastError(0);
	retp = LocalFree(argv);
	err = GetLastError();
	odprintf("LocalFree: %p (%ld)", retp, err);

done:
	exit(status);
}
