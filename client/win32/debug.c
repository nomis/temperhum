#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

void odprintf(const char *fmt, ...) {
		char buf[4096] = {};
		int ret;
		va_list args;

		va_start(args, fmt);
		ret = vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		if (ret < 0)
			OutputDebugString("Error in odprintf()");
		else
			OutputDebugString(buf);
}

