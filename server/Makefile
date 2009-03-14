.PHONY: all clean

CFLAGS=-Wall -Wextra -Wshadow -D_POSIX_C_SOURCE=200112L -D_ISOC99_SOURCE -D_SVID_SOURCE -O2
TEMPERHUM_OBJS=temperhum.o

all: temperhum
clean:
	rm -f temperhum *.o

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

temperhum.c: temperhum.h

temperhum: $(TEMPERHUM_OBJS) Makefile
	$(CC) $(CFLAGS) -lm -lrt -lrrd -o $@ $(TEMPERHUM_OBJS)
