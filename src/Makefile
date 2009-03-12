.PHONY: all clean

CFLAGS=-Wall -Wextra -Wshadow -D_POSIX_C_SOURCE=200112L -D_ISOC99_SOURCE -D_SVID_SOURCE -O2
TEMPERHUM_OBJS=comms.o readings.o temperhum.o
EEPROM_DUMP_OBJS=comms.o eeprom_dump.o

all: temperhum eeprom_dump
clean:
	rm -f temperhum eeprom_dump *.o

temperhum: $(TEMPERHUM_OBJS) Makefile
	gcc $(CFLAGS) -lm -lrt -lrrd -o temperhum $(TEMPERHUM_OBJS)

eeprom_dump: $(EEPROM_DUMP_OBJS) Makefile
	gcc $(CFLAGS) -lrt -o eeprom_dump $(EEPROM_DUMP_OBJS)
