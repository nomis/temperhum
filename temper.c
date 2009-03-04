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
	fprintf(stderr, "Init starting\n");
	WriteP1P0(1, "0110000");
	WriteP1P0(0, "00000000");
	fprintf(stderr, "Init done\n");
}

int main(int argc, char *argv[]) {
	temper_open(argv[1]);
	temper_init();

	while (1) {
		double foo = ReadSHT('T');
		printf("T = %f\n", foo);

		foo = ReadSHT('H');
		printf("\t\t\tH = %f\n", foo);
	}
	return 0;
}
