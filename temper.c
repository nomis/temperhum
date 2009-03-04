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

double round(double x);

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyUSB0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

char m_CHic;
int fd;

void Delay(int msec) {
	usleep(50 * msec);
}

void DTR(int set)
{
	int status = 0;
	ioctl(fd, TIOCMGET, &status);
	if (set)
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;

	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("ioctl");
		exit(1);
	}
}

void RTS(int set)
{
	int status = 0;
	if (ioctl(fd, TIOCMGET, &status) < 0) {
		perror("ioctl");
		exit(1);
	}

	if (set)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;
	if (ioctl(fd, TIOCMSET, &status) < 0) {
		perror("ioctl");
		exit(1);
	}
}

int CTS(void)
{
	int status = 0;
	ioctl(fd, TIOCMGET, &status);
	return (status & TIOCM_CTS);
}

void Sclk(int a) {
	if (m_CHic == 'T') {
		DTR(a == 0);
	} else if (m_CHic == 'R') {
		DTR(a == 1);
	}
}

void SDout(int ad01)
{
	if (m_CHic == 'T')
	{
		if (ad01 == 0)
			RTS(1);
		if (ad01 == 1)
			RTS(0);
	}
	if (m_CHic == 'R')
	{
		if (ad01 == 0)
			RTS(0);
		if (ad01 == 1)
			RTS(1);
	}
}

int SDin(void)
{
	int SDin = 0;

	SDout(1);
	Delay(50);
	Delay(50);
	int a = CTS();
	if (m_CHic == 'T')
	{
		if (!a)
			SDin = 1;
		else
			SDin = 0;
	}
	if (m_CHic != 'R')
		return SDin;
	if (!a)
		return 0;
	return 1;
}

void HiLowSCLK(void) {
	Delay(10);
	Sclk(1);
	Delay(20);
	Sclk(0);
	Delay(20);
}

void Start_IIC(void) {
	SDout(1);
	Delay(4);
	Sclk(1);
	Delay(40);
	SDout(0);
	Delay(30);
	Sclk(0);
}

void Stop_IIC(void) {
	SDout(0);
	Delay(50);
	Sclk(1);
	Delay(50);
	SDout(1);
	Delay(50);
}

void WriteP1P0(int P0123, char *dataS)
{
	Stop_IIC();
	Delay(100);
	Start_IIC();
	SDout(1);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	Delay(100);
	Sclk(1);
	Delay(100);
	SDin();
	HiLowSCLK();
	
	SDout(0);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	if (P0123 == 0)
	{
		SDout(0);
		HiLowSCLK();
		SDout(0);
		HiLowSCLK();
	}
	else if (P0123 == 1)
	{
		SDout(0);
		HiLowSCLK();
		SDout(1);
		HiLowSCLK();
	}
	else if (P0123 == 2)
	{
		SDout(1);
		HiLowSCLK();
		SDout(0);
		HiLowSCLK();
	}
	else if (P0123 == 3)
	{
		SDout(1);
		HiLowSCLK();
		SDout(1);
		HiLowSCLK();
	}
	Delay(100);
	Sclk(1);
	Delay(100);
	SDin();
	HiLowSCLK();

	while (*dataS != '\0') {
		if (*dataS == '1') {
			SDout(1);
		} else {
			SDout(0);
		}
		HiLowSCLK();
		Delay(100);
		dataS++;
	}
	Sclk(1);
	Delay(100);
	Delay(100);
	SDin();
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	Stop_IIC();
}

void Init(void)
{
	fprintf(stderr, "Init starting\n");
	m_CHic = 'R';
	WriteP1P0(1, "0110000");
	WriteP1P0(0, "00000000");
	fprintf(stderr, "Init done\n");
}


double Bin2Dec(const char *s) {
	double lDec = 0.0;

	if (s == NULL || strlen(s) == 0)
		s = "0";

	while (*s != '\0')
	{
		if (*s == '1')
			lDec += pow(2.0, strlen(s) - 1);
		s++;
	}
	return lDec;
}

double ReadTEMP(void)
{
	static char buf[16];
	double temperature;

	SDout(1);
	SDin();
	SDout(0);
	SDin();
	Start_IIC();
	SDout(1);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(0);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	SDout(1);
	HiLowSCLK();
	Sclk(1);
	Delay(100);
	int tt = SDin();
	HiLowSCLK();

	if (tt == 1)
	{
		if (m_CHic == 'T')
			m_CHic = 'R';
		else
			m_CHic = 'T';
	}
	if (tt == 1)
		return -1000.0;

	memset(buf, '\0', sizeof(buf));
	int i;

	for (i = 0; i < 16; i++) {
		int s = SDin();

		buf[i] = (s == 0 ? '0' : '1');
		HiLowSCLK();

		if (i == 7) {
			tt = SDin();
			Delay(100);
			HiLowSCLK();
		}
	}
	Sclk(0);
	Delay(1);

	SDout(0);
	HiLowSCLK();
	Stop_IIC();


	char str_msb[4];
	char str_lsb[9];
	char FuHao = buf[0];

	memcpy(str_msb, buf, 3);
	str_msb[3] = '\0';

	memcpy(str_lsb, buf+3, 8);
	str_lsb[8] = '\0';

	double msb = Bin2Dec(str_msb);
	double lsb = Bin2Dec(str_lsb);
	double tempdata = (msb * 256.0) + lsb;

	switch (FuHao)
	{
	case '0':
		temperature = tempdata * 0.125;
		break;

	case '1':
		temperature = -(2048.0 - tempdata) * 0.125;
		break;
	}

	fprintf(stderr, "Getting temperature done %16s %f\n", buf, temperature);

	return temperature;

/*                String str_data = data.ToString();
                String FuHao = Strings.Left(str_data, 1);
                String str_temp = Strings.Left(str_data, 11);
                String str_msb = Strings.Left(str_temp, 3);
                String str_lsb = Strings.Right(str_temp, 8);
                double msb = Bin2Dec(str_msb);
                double lsb = Bin2Dec(str_lsb);
                double tempdata = (msb * 256.0) + lsb;
                switch (FuHao)
                {
                    case "0":
                        ReadTEMP = tempdata * 0.125;
                        break;

                    case "1":
                        ReadTEMP = -(2048.0 - tempdata) * 0.125;
                        break;
                }


                return ReadTEMP;
*/
}

#include "hum.c"

int main(int argc, char **argv)
{
	struct termios oldtio,newtio;

//	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0) {
//		perror(MODEMDEVICE);
		perror(argv[1]);
		exit(-1);
	}


	tcgetattr(fd,&oldtio); /* save current port settings */

	memcpy(&newtio, &oldtio, sizeof(newtio));
/*	bzero(&newtio, sizeof(newtio));*/
	newtio.c_cflag |= (CLOCAL | CREAD);

	newtio.c_cflag &= ~PARENB;
	newtio.c_cflag &= ~CSTOPB;
	newtio.c_cflag &= ~CSIZE;
	newtio.c_cflag |= CS8;

	cfsetispeed(&newtio, BAUDRATE);
	cfsetospeed(&newtio, BAUDRATE);

	newtio.c_cflag |= CRTSCTS;

	newtio.c_iflag |= IGNPAR;
	newtio.c_oflag &= ~OPOST;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);

	Init();

	while (1) {
		double foo = ReadSHT('T');
		printf("T = %f\n", foo);
Delay(4);

		foo = ReadSHT('H');
		printf("\t\t\tH = %f\n", foo);
		usleep(1000000);
	}
	return 0;
}
