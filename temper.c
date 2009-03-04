#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "comms.h"

double ReadSHT(char TH);

void temper_start_IIC(void) {
	temper_switch(1, 0);
}

void temper_stop_IIC(void) {
	temper_switch(0, 1);
}

void temper_init(void) {
	printf("Init starting\n");

	temper_stop_IIC();
	temper_delay(100);
	temper_start_IIC();

	temper_write(0x9E, 8);
	temper_pause();

	temper_write(0x01, 8);
	temper_pause();

	temper_write(0x30, 7);
	temper_pause();

	temper_write(0x00, 1);

	temper_stop_IIC();
	temper_delay(100);
	temper_start_IIC();

	temper_write(0x9E, 8);
	temper_pause();

	temper_write(0x00, 8);
	temper_pause();

	temper_write(0x00, 8);
	temper_pause();

	temper_write(0x00, 1);
	temper_stop_IIC();

	printf("Init done\n");
}

int main(int argc, char *argv[]) {
	temper_open(argv[1]);
	temper_init();

	while (1) {
		double foo = ReadSHT('T');
		printf("\t\t\t\t\t\tT = %f\n", foo);

		foo = ReadSHT('H');
		printf("\t\t\t\t\t\t\t\t\tH = %f\n", foo);
sleep(1);
	}
	return 0;
}
