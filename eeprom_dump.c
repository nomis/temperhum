#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "comms.h"

int main(int argc, char *argv[]) {
	int addr;
	int err;
	unsigned int value;
	(void)argc;

	if (argc != 2) {
		printf("Usage: %s /dev/ttyUSB0\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	sht1x_open(argv[1]);

	for (addr = 0; addr < 256; addr++) {
		err = sht1x_command(CH341_ADDR, CH341_CMD_WRITE);
		if (err) {
			fprintf(stderr, "EEPROM write failed");
			exit(EXIT_FAILURE);
		}

		err = sht1x_write(addr);
		if (err) {
			fprintf(stderr, "EEPROM address failed");
			exit(EXIT_FAILURE);
		}

		err = sht1x_command(CH341_ADDR, CH341_CMD_READ);
		if (err) {
			fprintf(stderr, "EEPROM read failed");
			exit(EXIT_FAILURE);
		}

		value = sht1x_read(1);
		if (addr % 16 != 0)
			printf(", ");
		printf("0x%02x", value & 0xFF);
		if (addr % 16 == 15)
			printf(",\n");
		fflush(stdout);
	}

	sht1x_close();
	exit(EXIT_SUCCESS);
}
