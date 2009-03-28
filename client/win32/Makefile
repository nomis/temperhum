.PHONY: all clean version.h
.SUFFIXES:
.SUFFIXES: .c .o .rc

VER_WIN=0x0501
VER_IE=0x0500

CROSS_COMPILE=i686-mingw32-
CROSS_COMPILE_CFLAGS=
CC=gcc
DEFINE=-DWINVER=$(VER_WIN) -D_WIN32_WINNT=$(VER_WIN) -D_WIN32_IE=$(VER_IE)
CFLAGS=-Wall -Wextra -Wshadow -D_ISOC99_SOURCE $(DEFINE) -O2
LDFLAGS=-Wl,-subsystem,windows -lm -lws2_32
WINDRES=windres

ifeq ("$(shell uname -o)", "Cygwin")
	CROSS_COMPILE=
	CROSS_COMPILE_CFLAGS=-mno-cygwin 
endif

TEMPERHUM_OBJS=debug.o tray.o icon.o comms.o temperhum.o app.o

all: temperhum.exe
clean:
	rm -f temperhum.exe *.o version.h *.tmp

%.o: %.c Makefile
	$(CROSS_COMPILE)$(CC) $(CROSS_COMPILE_CFLAGS)$(CFLAGS) -c -o $@ $<

%.o: %.rc Makefile
	$(CROSS_COMPILE)$(WINDRES) $(DEFINE) -l 0x0809 -c 0xFDE9 -i $< -o $@

debug.o: debug.h
icon.o: debug.h icon.h
temperhum.o: config.h debug.h temperhum.h tray.h
tray.o: config.h debug.h tray.h icon.h temperhum.h digits.h
comms.o: config.h debug.h temperhum.h tray.h
app.o: version.h

version.h:
	@./mkversion "$(VER_WIN)" "$(VER_IE)"

temperhum.exe: $(TEMPERHUM_OBJS) Makefile
	$(CROSS_COMPILE)$(CC) $(CROSS_COMPILE_CFLAGS)$(CFLAGS) -o temperhum.exe $(TEMPERHUM_OBJS) $(LDFLAGS)
