.PHONY: all clean

CC=i686-mingw32-gcc
CFLAGS=-Wall -Wextra -Wshadow -D_ISOC99_SOURCE -D_WIN32_WINNT=0x0501 -D_WIN32_IE=0x0500 -O2
LDFLAGS=-lws2_32 -lm
TEMPERHUM_OBJS=debug.o tray.o icon.o temperhum.o

all: temperhum.exe
clean:
	rm -f temperhum.exe *.o

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

debug.c: debug.h
icon.c: debug.h icon.h
temperhum.c: config.h debug.h temperhum.h tray.h
tray.c: debug.h tray.h icon.h temperhum.h digits.h

temperhum.exe: $(TEMPERHUM_OBJS) Makefile
	$(CC) $(CFLAGS) -o temperhum.exe $(TEMPERHUM_OBJS) $(LDFLAGS)
