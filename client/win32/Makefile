.PHONY: all clean

CROSS_COMPILE=i686-mingw32-
CC=gcc
CFLAGS=-Wall -Wextra -Wshadow -D_ISOC99_SOURCE -D_WIN32_WINNT=0x0501 -D_WIN32_IE=0x0500 -O2
LDFLAGS=-lws2_32 -lm
WINDRES=windres
TEMPERHUM_OBJS=debug.o tray.o icon.o comms.o temperhum.o exe.o

all: temperhum.exe
clean:
	rm -f temperhum.exe *.o

%.o: %.c Makefile
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c -o $@ $<

debug.o: debug.h
icon.o: debug.h icon.h
temperhum.o: config.h debug.h temperhum.h tray.h
tray.o: config.h debug.h tray.h icon.h temperhum.h digits.h
comms.o: config.h debug.h temperhum.h tray.h

exe.o: exe.rc
	$(CROSS_COMPILE)$(WINDRES) -i $< -o $@

temperhum.exe: $(TEMPERHUM_OBJS) Makefile
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o temperhum.exe $(TEMPERHUM_OBJS) $(LDFLAGS)
