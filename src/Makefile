.PHONY: all clean

CFLAGS=-Wall -Wextra -Wshadow -D_POSIX_C_SOURCE=200112L -D_ISOC99_SOURCE -D_SVID_SOURCE -O2
TEMPERHUM_OBJS=comms.o readings.o temperhum.o
EEPROM_DUMP_OBJS=comms.o eeprom_dump.o

all: temperhum eeprom_dump
clean:
	rm -f temperhum eeprom_dump *.o

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

temperhum.o: readings.h comms.h
readings.o: readings.h comms.h
comms.o: comms.h

temperhum: $(TEMPERHUM_OBJS) Makefile
	$(CC) $(CFLAGS) -lm -lrt -lrrd -o $@ $(TEMPERHUM_OBJS)

eeprom_dump.o: comms.h

eeprom_dump: $(EEPROM_DUMP_OBJS) Makefile
	$(CC) $(CFLAGS) -lrt -o $@ $(EEPROM_DUMP_OBJS)
