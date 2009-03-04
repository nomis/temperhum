#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define BAUDRATE 9600

#if 1
# define TEMPER_0 1
# define TEMPER_1 0
#else
# define TEMPER_0 0
# define TEMPER_1 1
#endif

#define COMMS_C
#include "comms.h"

static int fd = -1;
static int ok = 0;

/* Input */
int temper_in(void) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/CTS");
		exit(EXIT_FAILURE);
	}

	return (status & TIOCM_CTS) ? TEMPER_0 : TEMPER_1;
}

int temper_get(void) {
	temper_out(1);
	temper_delay(100);
	return temper_in();
}

/* Output */
void temper_clock(int v) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/DTR");
		exit(EXIT_FAILURE);
	}

	if (v == TEMPER_0) {
		if ((status & TIOCM_DTR) != 0)
			printf("clk 1 already set!\n");
	} else {
		if ((status & TIOCM_DTR) == 0)
			printf("clk 0 already set!\n");
	}

//printf("clk %d\n", v == TEMPER_0);
	if (v == TEMPER_0)
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;

	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/DTR");
		exit(EXIT_FAILURE);
	}
}

void temper_clock_signal(void) {
	temper_delay(10);
	temper_clock(1);
	temper_delay(20);
	temper_clock(0);
	temper_delay(20);
}

void temper_out(int v) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/RTS");
		exit(EXIT_FAILURE);
	}

	if ((status & TIOCM_DTR) != 0) {
	if (v == TEMPER_0) {
		if ((status & TIOCM_RTS) != 0 && !ok)
			printf("out 0->1 while clock high!\n");
	} else {
		if ((status & TIOCM_RTS) == 0 && !ok)
			printf("out 1->0 while clock high!\n");
	}
	}

//printf("out %d\n", v == TEMPER_0);
	if (v == TEMPER_0)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/RTS");
		exit(EXIT_FAILURE);
	}
}

/* Timing */
void temper_delay(int n) {
	struct timespec req;

	req.tv_sec = 0;
// TODO 10000
	req.tv_nsec = 50000 * n;

//printf("dly %d\n", n);
	if (nanosleep(&req, NULL) != 0) {
		perror("nanosleep");
		exit(EXIT_FAILURE);
	}
}

/* Comms */
void temper_switch(int rising, int falling) {
ok = 1;
	temper_out(rising);
	temper_delay(10);
	temper_clock(1);
	temper_delay(40);
	temper_out(falling);
	temper_clock(0);
	temper_delay(20);
ok = 0;
}

int temper_wait(int timeout) {
	int waited = 0;

printf("...\n");

#if 0
	while (waited++ < timeout) {
		if (temper_in()) {
			waited = -1;
			break;
		}
		temper_delay(100);
	}

	if (waited != -1)
		return -1;
	waited = 0;
#endif

	temper_delay(100);
	temper_write(0x01, 1);

	temper_delay(100);
	while (waited++ < timeout) {
		temper_delay(100);
		if (!temper_in())
			return waited;
	}
	return -1;
}

void temper_write(int data, int len) {
	while (len-- > 0) {
		temper_out(data >> len & 1);
		temper_clock_signal();
	}
}

/* Device */
void temper_open(char *dev) {
	struct termios tio;

	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd == -1) {
		perror(dev);
		exit(EXIT_FAILURE);
	}

	if (tcgetattr(fd, &tio) != 0) {
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}

	tio.c_cflag |= (CLOCAL | CREAD);

	tio.c_cflag &= ~PARENB;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8;

	cfsetispeed(&tio, BAUDRATE);
	cfsetospeed(&tio, BAUDRATE);

	tio.c_iflag |= IGNPAR;
	tio.c_oflag &= ~OPOST;

	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	tio.c_cc[VTIME] = 1;
	tio.c_cc[VMIN] = 1;

	if (tcflush(fd, TCIFLUSH) != 0) {
		perror("tcflush");
		exit(EXIT_FAILURE);
	}

	if (tcsetattr(fd, TCSANOW, &tio) != 0) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}

	temper_clock(0);
}

void temper_close(void) {
	close(fd);
	fd = -1;
}
