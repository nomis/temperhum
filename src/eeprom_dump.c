/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "comms.h"

int main(int argc, char *argv[]) {
	struct sht1x_device dev;
	int addr;
	int err;
	unsigned int value;

	if (argc != 2) {
		printf("Usage: %s 1-2.3.4\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	strncpy(dev.name, argv[1], sizeof(dev.name));
	sht1x_open(&dev);

	for (addr = 0; addr < 256; addr++) {
		err = sht1x_command(&dev, CH341_ADDR, CH341_CMD_WRITE);
		if (err) {
			fprintf(stderr, "EEPROM write failed");
			exit(EXIT_FAILURE);
		}

		err = sht1x_write(&dev, addr);
		if (err) {
			fprintf(stderr, "EEPROM address failed");
			exit(EXIT_FAILURE);
		}

		err = sht1x_command(&dev, CH341_ADDR, CH341_CMD_READ);
		if (err) {
			fprintf(stderr, "EEPROM read failed");
			exit(EXIT_FAILURE);
		}

		value = sht1x_read(&dev, 1);
		if (addr % 16 != 0)
			printf(", ");
		printf("0x%02x", value & 0xFF);
		if (addr % 16 == 15)
			printf(",\n");
		fflush(stdout);
	}

	sht1x_close(&dev);
	exit(EXIT_SUCCESS);
}
