.PHONY: all clean version.h
.SUFFIXES:
.SUFFIXES: .c .o .rc

CROSS_COMPILE=i686-mingw32-
CC=gcc
CFLAGS=-Wall -Wextra -Wshadow -D_ISOC99_SOURCE -D_WIN32_WINNT=0x0501 -D_WIN32_IE=0x0500 -O2
LDFLAGS=-lws2_32 -lm
WINDRES=windres
TEMPERHUM_OBJS=debug.o tray.o icon.o comms.o temperhum.o exe.o

all: temperhum.exe
clean:
	rm -f temperhum.exe *.o version.h *.tmp

%.o: %.c Makefile
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.rc Makefile
	$(CROSS_COMPILE)$(WINDRES) -i $< -o $@

debug.o: debug.h
icon.o: debug.h icon.h
temperhum.o: config.h debug.h temperhum.h tray.h
tray.o: config.h debug.h tray.h icon.h temperhum.h digits.h
comms.o: config.h debug.h temperhum.h tray.h
exe.o: version.h

version.h: Makefile ../../.git
	@rm -f version.h.tmp version.h
	@touch version.h.tmp
	@echo "#define GIT_AUTHOR \"$(GIT_AUTHOR_NAME)\\0\"" >> version.h.tmp
	@git update-index --refresh --unmerged >/dev/null \
		|| (echo "#define GIT_REV \"`git rev-parse --verify HEAD`-dirty\\0\"" >> version.h.tmp; \
			echo "#define GIT_FILEFLAGS_NORMAL VS_FF_PATCHED" >> version.h.tmp; \
			echo "#define GIT_FILEFLAGS_DEBUG VS_FF_DEBUG|VS_FF_PATCHED" >> version.h.tmp) \
		|| (echo "#define GIT_REV \"`git rev-parse --verify HEAD`\\0\"" >> version.h.tmp; \
			echo "#define GIT_FILEFLAGS_NORMAL 0" >> version.h.tmp; \
			echo "#define GIT_FILEFLAGS_DEBUG VS_FF_DEBUG" >> version.h.tmp)
	@echo "#define RC_COMMENTS \"Built `date --utc +%FT%TZ` by `whoami`@`hostname`\\0\"" >> version.h.tmp
	@mv version.h.tmp version.h

temperhum.exe: $(TEMPERHUM_OBJS) Makefile
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o temperhum.exe $(TEMPERHUM_OBJS) $(LDFLAGS)
