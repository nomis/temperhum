#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>

#define BAUDRATE 9600

#if 1
# define TEMPER_0 1
# define TEMPER_1 0
#else
# define TEMPER_0 0
# define TEMPER_1 1
#endif

#include "comms.h"

static int fd;

void temper_delay(int n) {
	struct timespec req;

	req.tv_sec = 0;
	req.tv_nsec = 50000 * n;

	if (nanosleep(&req, NULL) != 0) {
		perror("nanosleep");
		exit(EXIT_FAILURE);
	}
}

void temper_DTR(int set) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/DTR");
		exit(EXIT_FAILURE);
	}

	if (set)
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;

	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/DTR");
		exit(EXIT_FAILURE);
	}
}

void temper_RTS(int set) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/RTS");
		exit(EXIT_FAILURE);
	}

	if (set)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/RTS");
		exit(EXIT_FAILURE);
	}
}

int CTS(void) {
	int status = 0;

	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/CTS");
		exit(EXIT_FAILURE);
	}

	return (status & TIOCM_CTS);
}

void temper_clock(int a) {
	temper_DTR(a == TEMPER_0);
}

void temper_out(int a) {
	temper_RTS(a == TEMPER_0);
}

int temper_in(void) {
	temper_out(1);
	temper_delay(50);
//	temper_delay(50);
	return CTS() ? TEMPER_0 : TEMPER_1;
}

void temper_clock_signal(void) {
	temper_delay(10);
	temper_clock(1);
	temper_delay(20);
	temper_clock(0);
	temper_delay(20);
}

void Start_IIC(void) {
	temper_out(1);
	temper_delay(4);
	temper_clock(1);
	temper_delay(40);
	temper_out(0);
	temper_delay(30);
	temper_clock(0);
}

void Stop_IIC(void) {
	temper_out(0);
	temper_delay(50);
	temper_clock(1);
	temper_delay(50);
	temper_out(1);
	temper_delay(50);
}

void WriteP1P0(int P0123, char *dataS)
{
	Stop_IIC();
	temper_delay(100);
	Start_IIC();
	temper_out(1);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	temper_out(1);
	temper_clock_signal();
	temper_out(1);
	temper_clock_signal();
	temper_out(1);
	temper_clock_signal();
	temper_out(1);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	temper_delay(100);
	temper_clock(1);
	temper_delay(100);
	temper_in();
	temper_clock_signal();
	
	temper_out(0);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	if (P0123 == 0)
	{
		temper_out(0);
		temper_clock_signal();
		temper_out(0);
		temper_clock_signal();
	}
	else if (P0123 == 1)
	{
		temper_out(0);
		temper_clock_signal();
		temper_out(1);
		temper_clock_signal();
	}
	else if (P0123 == 2)
	{
		temper_out(1);
		temper_clock_signal();
		temper_out(0);
		temper_clock_signal();
	}
	else if (P0123 == 3)
	{
		temper_out(1);
		temper_clock_signal();
		temper_out(1);
		temper_clock_signal();
	}
	temper_delay(100);
	temper_clock(1);
	temper_delay(100);
	temper_in();
	temper_clock_signal();

	while (*dataS != '\0') {
		if (*dataS == '1') {
			temper_out(1);
		} else {
			temper_out(0);
		}
		temper_clock_signal();
		temper_delay(100);
		dataS++;
	}
	temper_clock(1);
//	temper_delay(100);
	temper_delay(100);
	temper_in();
	temper_clock_signal();
	temper_out(0);
	temper_clock_signal();
	Stop_IIC();
}

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

	tio.c_cc[VTIME]    = 1;
	tio.c_cc[VMIN]     = 1;

	if (tcflush(fd, TCIFLUSH) != 0) {
		perror("tcflush");
		exit(EXIT_FAILURE);
	}

	if (tcsetattr(fd, TCSANOW, &tio) != 0) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
}
