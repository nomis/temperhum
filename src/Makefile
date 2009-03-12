CFLAGS=-Wall -Wextra -Wshadow -D_POSIX_C_SOURCE=200112L -D_ISOC99_SOURCE -D_SVID_SOURCE -O2
all: temperhum eeprom_dump
clean:
	rm -f temperhum eeprom_dump
temperhum: comms.c readings.c temperhum.c Makefile
	gcc $(CFLAGS) -lm -lrt -lrrd -o temperhum comms.c readings.c temperhum.c
eeprom_dump: comms.c eeprom_dump.c Makefile
	gcc $(CFLAGS) -lrt -o eeprom_dump comms.c eeprom_dump.c
