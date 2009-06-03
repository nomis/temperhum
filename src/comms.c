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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define BAUDRATE 9600
#define DELAY 2500000 /* 2.5ms */

#define COMMS_C
#include "comms.h"

static sigjmp_buf saved;

static unsigned char crc_table[256] = {
	0x00, 0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea, 0x7d, 0x4c, 0x1f, 0x2e,
	0x43, 0x72, 0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4, 0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0x0f, 0x5c, 0x6d,
	0x86, 0xb7, 0xe4, 0xd5, 0x42, 0x73, 0x20, 0x11, 0x3f, 0x0e, 0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8,
	0xc5, 0xf4, 0xa7, 0x96, 0x01, 0x30, 0x63, 0x52, 0x7c, 0x4d, 0x1e, 0x2f, 0xb8, 0x89, 0xda, 0xeb,
	0x3d, 0x0c, 0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa, 0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71, 0x22, 0x13,
	0x7e, 0x4f, 0x1c, 0x2d, 0xba, 0x8b, 0xd8, 0xe9, 0xc7, 0xf6, 0xa5, 0x94, 0x03, 0x32, 0x61, 0x50,
	0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e, 0x1d, 0x2c, 0x02, 0x33, 0x60, 0x51, 0xc6, 0xf7, 0xa4, 0x95,
	0xf8, 0xc9, 0x9a, 0xab, 0x3c, 0x0d, 0x5e, 0x6f, 0x41, 0x70, 0x23, 0x12, 0x85, 0xb4, 0xe7, 0xd6,
	0x7a, 0x4b, 0x18, 0x29, 0xbe, 0x8f, 0xdc, 0xed, 0xc3, 0xf2, 0xa1, 0x90, 0x07, 0x36, 0x65, 0x54,
	0x39, 0x08, 0x5b, 0x6a, 0xfd, 0xcc, 0x9f, 0xae, 0x80, 0xb1, 0xe2, 0xd3, 0x44, 0x75, 0x26, 0x17,
	0xfc, 0xcd, 0x9e, 0xaf, 0x38, 0x09, 0x5a, 0x6b, 0x45, 0x74, 0x27, 0x16, 0x81, 0xb0, 0xe3, 0xd2,
	0xbf, 0x8e, 0xdd, 0xec, 0x7b, 0x4a, 0x19, 0x28, 0x06, 0x37, 0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91,
	0x47, 0x76, 0x25, 0x14, 0x83, 0xb2, 0xe1, 0xd0, 0xfe, 0xcf, 0x9c, 0xad, 0x3a, 0x0b, 0x58, 0x69,
	0x04, 0x35, 0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93, 0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48, 0x1b, 0x2a,
	0xc1, 0xf0, 0xa3, 0x92, 0x05, 0x34, 0x67, 0x56, 0x78, 0x49, 0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef,
	0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77, 0x24, 0x15, 0x3b, 0x0a, 0x59, 0x68, 0xff, 0xce, 0x9d, 0xac,
};

/* Timing */
void sht1x_delay(void) {
	struct timespec req;

	req.tv_sec = 0;
	req.tv_nsec = DELAY;

	if (clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL) != 0) {
		perror("clock_nanosleep");
		exit(EXIT_FAILURE);
	}
}

void sht1x_startup_delay(void) {
	struct timespec req;

	req.tv_sec = 0;
	req.tv_nsec = 15000000; /* 15ms */

	if (clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL) != 0) {
		perror("clock_nanosleep");
		exit(EXIT_FAILURE);
	}
}

void sht1x_alarm(int signum) {
	(void)signum;

	/* Timer expired; jump */
	longjmp(saved, 1);
}

/* Input */
int sht1x_in(struct sht1x_device *dev) {
	int status = 0;

	sht1x_delay();

	if (ioctl(dev->fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/CTS");
		exit(EXIT_FAILURE);
	}

	return (status & TIOCM_CTS) ? 1 : 0;
}

int sht1x_in_wait(struct sht1x_device *dev) {
	/* Check for ready state */
	if (sht1x_in(dev)) {
		struct itimerval timeout;
		timeout.it_interval.tv_sec = 0;
		timeout.it_interval.tv_usec = 0;
		timeout.it_value.tv_sec = 3;
		timeout.it_value.tv_usec = 500000; /* 500ms */

		/* Jump point */
		if (sigsetjmp(saved, 1) == 0) {
			/* Start timer */
			signal(SIGALRM, sht1x_alarm);
			if (setitimer(ITIMER_REAL, &timeout, NULL) != 0) {
				perror("setitimer");
				exit(EXIT_FAILURE);
			}

			/* Wait for activity on CTS */
			ioctl(dev->fd, TIOCMIWAIT, TIOCM_CTS);
	
			/* Stop timer */
			timeout.it_value.tv_sec = 0;
			timeout.it_value.tv_usec = 0;
			if (setitimer(ITIMER_REAL, &timeout, NULL) != 0) {
				perror("setitimer");
				exit(EXIT_FAILURE);
			}
		} else {
			/* Timer expired */
		}

		/* Check for ready state */
		return sht1x_in(dev);
	}
	return 0;
}

/* Output */
void sht1x_sck(struct sht1x_device *dev, int v) {
	int status = 0;

	if (ioctl(dev->fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/DTR");
		exit(EXIT_FAILURE);
	}

	if (v)
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;

	if (ioctl(dev->fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/DTR");
		exit(EXIT_FAILURE);
	}

	sht1x_delay();
}

void sht1x_out(struct sht1x_device *dev, int v) {
	int status = 0;

	if (ioctl(dev->fd, TIOCMGET, &status) < 0) {
		perror("TIOCMGET/RTS");
		exit(EXIT_FAILURE);
	}

	if (v)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	if (ioctl(dev->fd, TIOCMSET, &status) < 0) {
		perror("TIOCMSET/RTS");
		exit(EXIT_FAILURE);
	}

	sht1x_delay();
}

/* Comms */
void sht1x_conn_reset(struct sht1x_device *dev) {
	int i;

	/* Raise DATA */
	sht1x_out(dev, 1);

	/* Lower SCK */
	sht1x_sck(dev, 0);

	/* Toggle SCK 9+ times */
	for (i = 0; i < 9; i++) {
		sht1x_sck(dev, 1);
		sht1x_sck(dev, 0);
	}
}

void sht1x_trans_start(struct sht1x_device *dev, int part1, int part2) {
	if (part1) {
		/* Raise DATA */
		sht1x_out(dev, 1);

		/* Raise SCK */
		sht1x_sck(dev, 1);

		/* Lower data */
		sht1x_out(dev, 0);

		/* Toggle SCK */
		sht1x_sck(dev, 0);
	}
	if (part2) {
		sht1x_sck(dev, 1);

		/* Raise data */
		sht1x_out(dev, 1);

		/* Lower SCK */
		sht1x_sck(dev, 0);
	}
}

/* Response:
 *   MSB = Success/Failure (timeout)
 *   --- = Checksum
 *   --- = Value MSB
 *   LSB = Value LSB
 */
unsigned int sht1x_read(struct sht1x_device *dev, int bytes) {
	unsigned int v = 0xFF000000;
	unsigned char orig_crc;
#ifdef DEBUG
	unsigned int d_v;
#endif
	int bits;

	if (bytes < 1 || bytes > 2)
		return v;

	bits = (bytes + 1) * 8;

	/* Raise DATA waiting for response */
	sht1x_out(dev, 1);

	/* Wait for response */
	if (sht1x_in_wait(dev))
		return v;

	/* Read MSB, LSB and checksum */
	do {
		/* Raise DATA */
		sht1x_out(dev, 1);

		/* Toggle SCK */
		sht1x_sck(dev, 1);
		sht1x_sck(dev, 0);

		/* Read bit */
		bits--;
		v |= sht1x_in(dev) << (bits - 1);

		/* Write ACK after each byte */
		if ((bits & 7) == 0) {
			if (bits > 0) {
				/* Lower DATA */
				sht1x_out(dev, 0);
			} else {
				/* Raise DATA */
				sht1x_out(dev, 0);
			}

			/* Toggle SCK */
			sht1x_sck(dev, 1);
			sht1x_sck(dev, 0);
		}
	} while (bits > 0);

#ifdef DEBUG
	d_v = v;
#endif
	orig_crc = dev->crc;

	/* Calculate CRC */
	if (bytes >= 2)
		dev->crc = crc_table[dev->crc ^ (v >> 16 & 0x000000FF)];
	if (bytes >= 1)
		dev->crc = crc_table[dev->crc ^ (v >> 8 & 0x000000FF)];

	/* Reverse checksum bits */
	v = (v & 0xFFFFFF00)
		| (v >> 7 & 0x01)
		| (v >> 5 & 0x02)
		| (v >> 3 & 0x04)
		| (v >> 1 & 0x08)
		| (v << 1 & 0x10)
		| (v << 3 & 0x20)
		| (v << 5 & 0x40)
		| (v << 7 & 0x80);

	/* Checksum OK */
	if ((v & 0xFE) == (dev->crc & 0xFE))
		v = (v & 0x00FFFFFF);

#ifdef DEBUG
	fprintf(stderr, "%02x/%02x/%08x/%02x/%02x\n", dev->fd, orig_crc, (v & 0xFF000000) | (d_v & 0x00FFFFFF), v & 0xFF, dev->crc);
#endif

	/* Move checksum to before MSB/LSB */
	v = (v & 0xFF000000) | (v >> 8 & 0x0000FFFF) | (v << 16 & 0x00FF0000);

	return v;
}

int sht1x_write(struct sht1x_device *dev, unsigned char data) {
	int err;
	int bits = 8;

	/* Calculate CRC */
	dev->crc = crc_table[dev->crc ^ data];

	while (bits-- > 0) {
		/* Set DATA */
		sht1x_out(dev, data >> bits & 1);

		/* Toggle SCK */
		sht1x_sck(dev, 1);
		sht1x_sck(dev, 0);
	}

	/* Raise DATA for ACK */
	sht1x_out(dev, 1);

	/* Raise SCK for ACK */
	sht1x_sck(dev, 1);

	/* Read ACK */
	err = sht1x_in(dev);

	/* Lower SCK */
	sht1x_sck(dev, 0);

	return err;
}

/* Control */
int sht1x_command(struct sht1x_device *dev, int addr, int cmd) {
	/* Start transmission */
	sht1x_trans_start(dev, 1, addr == SHT1X_ADDR ? 1 : 0);

	/* Reset CRC */
	dev->crc = dev->crc_init;
	dev->crc = (dev->crc >> 7 & 0x01)
		| (dev->crc >> 5 & 0x02)
		| (dev->crc >> 3 & 0x04)
		| (dev->crc >> 1 & 0x08)
		| (dev->crc << 1 & 0x10)
		| (dev->crc << 3 & 0x20)
		| (dev->crc << 5 & 0x40)
		| (dev->crc << 7 & 0x80);

	/* Write command and read ACK */
	return sht1x_write(dev, (addr << 5 & 0xE0) | (cmd & 0x1F));
}

int sht1x_device_reset(struct sht1x_device *dev) {
	int err;

	/* Assumption: it'll take more than 11ms
     * to bring up USB, so the chip is in
     * Sleep State already. */

	/* Reset comms */
	sht1x_conn_reset(dev);

	/* Status register will be reset before the CRC is next used */
	dev->crc_init = SHT1X_CRC_INIT;

	/* Soft reset */
	err = sht1x_command(dev, SHT1X_ADDR, SHT1X_CMD_S_RESET);

	/* Wait */
	sht1x_startup_delay();

	return err;
}

struct sht1x_status sht1x_read_status(struct sht1x_device *dev) {
	struct sht1x_status status = { 0, 0, 0, 0, 0 };
	unsigned int resp;
	int err;

	err = sht1x_command(dev, SHT1X_ADDR, SHT1X_CMD_R_SR);
	if (err)
        return status;

    resp = sht1x_read(dev, 1);
	if ((resp & 0xFF000000) == 0) {
		status.valid = 1;

		status.low_battery = (resp >> 6 & 1);
		status.heater = (resp >> 2 & 1);
		status.no_reload = (resp >> 1 & 1);
		status.low_resolution = (resp & 1);
	}

	return status;
}

int sht1x_write_status(struct sht1x_device *dev, struct sht1x_status status) {
	unsigned char req = 0;
	int err;

	err = sht1x_command(dev, SHT1X_ADDR, SHT1X_CMD_W_SR);
	if (err)
        return err;

	req = 0;
	if (status.heater)
		req |= 1 << 2;
	if (status.no_reload)
		req |= 1 << 1;
	if (status.low_resolution)
		req |= 1;

	dev->crc_init = req & 0x0F;

	err = sht1x_write(dev, req);
	if (err)
		sht1x_device_reset(dev);
	return err;
}

int ttyUSB(const struct dirent *entry) {
	return !strncmp(entry->d_name, "ttyUSB", 6);
}

/* Device */
void sht1x_open(struct sht1x_device *dev) {
	struct termios tio;
	char path[4096];
	struct dirent **namelist;
	int names, i;

	snprintf(path, 4096, "/sys/bus/usb/devices/%s:1.0", dev->name);
	names = scandir(path, &namelist, ttyUSB, alphasort);
	if (names == -1) {
		perror(path);
		exit(EXIT_FAILURE);
	}
	if (names == 0) {
		fprintf(stderr, "No ttyUSB devices found for %s\n", dev->name);
		exit(EXIT_FAILURE);
	}

	snprintf(path, 4096, "/dev/%s", namelist[0]->d_name);
	dev->fd = open(path, O_RDWR | O_NOCTTY);
	if (dev->fd == -1) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < names; i++)
		free(namelist[i]);
	free(namelist);

	if (tcgetattr(dev->fd, &tio) != 0) {
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

	if (tcflush(dev->fd, TCIFLUSH) != 0) {
		perror("tcflush");
		exit(EXIT_FAILURE);
	}

	if (tcsetattr(dev->fd, TCSANOW, &tio) != 0) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
}

void sht1x_close(struct sht1x_device *dev) {
	if (close(dev->fd) != 0) {
		perror("close");
		exit(EXIT_FAILURE);
	}

	dev->fd = -1;
}
