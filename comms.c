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

/* Input */
int temper_in(void) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/CTS");
		exit(EXIT_FAILURE);
	}

	return (status & TIOCM_CTS) ? TEMPER_0 : TEMPER_1;
}

unsigned int temper_read(int n) {
	unsigned int i = n;
	unsigned int v = 0;

	do {
		/* why is this required?
		 * most of the time it'll already be at 1 */
		temper_out(1);
		temper_delay(100); /* wait a bit for it to be ready */
		v |= temper_in() << --i;

		if (i != 0)
			temper_write_simple(0x01, 1);

		if (i > 0 && (i & 7) == 0) {
			temper_write_complex(0x02, 1);
			temper_write_simple(0x00, 1);
		}
	} while (i > 0);

	return v;
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

	if (v == TEMPER_0)
		status |= TIOCM_DTR;
	else if (v == TEMPER_1)
		status &= ~TIOCM_DTR;
	else {
		fprintf(stderr, "Internal error\n");
		exit(EXIT_FAILURE);
	}

	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/DTR");
		exit(EXIT_FAILURE);
	}
}

void temper_out(int v) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/RTS");
		exit(EXIT_FAILURE);
	}

	if (v == TEMPER_0)
		status |= TIOCM_RTS;
	else if (v == TEMPER_1)
		status &= ~TIOCM_RTS;
	else {
		fprintf(stderr, "Internal error\n");
		exit(EXIT_FAILURE);
	}

	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/RTS");
		exit(EXIT_FAILURE);
	}
}

/* Timing */
void temper_delay(int n) {
	struct timespec req;

	req.tv_sec = 0;
	req.tv_nsec = 50000 * n;

	if (nanosleep(&req, NULL) != 0) {
		perror("nanosleep");
		exit(EXIT_FAILURE);
	}
}

/* Comms */
void temper_clocked_out(int rising, int falling) {
	temper_out(rising);
	temper_delay(10);

	temper_clock(1);
	temper_delay(20);

	if (rising != falling) {
		temper_delay(20);
		temper_out(falling);
	}

	temper_clock(0);
	temper_delay(20);
}

int temper_wait(int timeout) {
	int waited = 0;

	temper_write_simple(0x01, 1);

	while (waited++ < timeout) {
		temper_delay(100);

		if (!temper_in())
			return waited;
	}
	return -1;
}

void temper_write_simple(unsigned int data, int len) {
	while (len-- > 0)
		temper_clocked_out(data >> len & 1, data >> len & 1);
}

void temper_write_complex(unsigned int data, int len) {
	while (len-- > 0)
		temper_clocked_out(data >> (2*len+1) & 1, data >> (2*len) & 1);
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
