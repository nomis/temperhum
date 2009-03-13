.PHONY: all clean

CC=i686-mingw32-gcc
CFLAGS=-Wall -Wextra -Wshadow -D_ISOC99_SOURCE -O2
LDFLAGS=-lws2_32 -lm
TEMPERHUM_OBJS=debug.o tray.o icon.o temperhum.o

all: temperhum.exe
clean:
	rm -f temperhum.exe *.o

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

temperhum.exe: $(TEMPERHUM_OBJS) Makefile
	$(CC) $(CFLAGS) -o temperhum.exe $(TEMPERHUM_OBJS) $(LDFLAGS)
