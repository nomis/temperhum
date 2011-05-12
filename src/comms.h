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

/* SHT */
	/* 3 bits */
#define SHT1X_ADDR				0x00

	/* 5 bits */
#define SHT1X_CMD_M_TEMP		0x03
#define SHT1X_CMD_M_RH			0x05
#define SHT1X_CMD_R_SR			0x07
#define SHT1X_CMD_W_SR			0x06
#define SHT1X_CMD_S_RESET		0x1E

	/* 4 bits */
#define SHT1X_CRC_INIT			0x00


/* CH341 EEPROM */
	/* 3 bits */
#define CH341_ADDR				0x05

	/* 5 bits */
#define CH341_CMD_WRITE			0x00
#define CH341_CMD_READ			0x01


struct sht1x_status {
	int valid;

	unsigned int low_battery;
	unsigned int heater;
	unsigned int no_reload;
	unsigned int low_resolution;
};

struct sht1x_device {
	char name[4096];
	char rrdfile[4096];
	unsigned char crc_init;
	unsigned char crc;
	int fd;
	double tc_offset;
	struct sht1x_status status;
	struct sht1x_device *next;
};


#ifdef COMMS_C

/* Timing */
void sht1x_delay(void);
void sht1x_startup_delay(void);
void sht1x_alarm(int signum);

/* Input */
int sht1x_in(struct sht1x_device *dev);
int sht1x_in_wait(struct sht1x_device *dev);

/* Output */
void sht1x_sck(struct sht1x_device *dev, int v);
void sht1x_out(struct sht1x_device *dev, int v);

/* Comms */
void sht1x_trans_start(struct sht1x_device *dev, int part1, int part2);
void sht1x_conn_reset(struct sht1x_device *dev);

#endif

/* Comms */
unsigned int sht1x_read(struct sht1x_device *dev, int bytes);
int sht1x_write(struct sht1x_device *dev, unsigned char data);

/* Control */
int sht1x_command(struct sht1x_device *dev, int addr, int cmd);
int sht1x_device_reset(struct sht1x_device *dev);

/* Device */
void sht1x_open(struct sht1x_device *dev);
struct sht1x_status sht1x_read_status(struct sht1x_device *dev);
int sht1x_write_status(struct sht1x_device *dev, struct sht1x_status status);
void sht1x_close(struct sht1x_device *dev);
