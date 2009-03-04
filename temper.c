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

void temper_init(void) {
	printf("Init starting\n");


	temper_switch(1, 0);
	temper_write(0x9E, 8);

	temper_switch(0, 1);
	temper_write(0x01, 8);

	temper_switch(0, 1);
	temper_write(0x60, 8);

	temper_switch(0, 1);
	temper_write(0x00, 1);

	temper_switch(0, 1);


	temper_switch(1, 0);
	temper_write(0x9E, 8);

	temper_switch(0, 1);
	temper_write(0x00, 8);

	temper_switch(0, 1);
	temper_write(0x00, 8);

	temper_switch(0, 1);
	temper_write(0x00, 1);

	temper_switch(0, 1);


	printf("Init done\n");
}

int main(int argc, char *argv[]) {
	(void)argc;

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
