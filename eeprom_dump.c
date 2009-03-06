/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
