#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comms.h"

static double lasttemp = 0;

double ReadSHT(char TH) {
			int xxx=0, bad=0;

	double tempdata=0;

temper_write_complex(0x02, 1);
temper_write_complex(0x01, 1);

			if (TH == 'T')
				temper_write_simple(0x03, 8);
			else if (TH == 'H')
				temper_write_simple(0x05, 8);

			xxx = temper_wait(500);
			if (xxx < 0) {
				bad = 1;
				printf("bad 1\n");
			} else {
				printf("waited %d\n", xxx);
			}

			xxx = temper_read(1);
			if (xxx == 1) {
				bad = 1;
				printf("bad 2\n");
			}

		int tempreading = temper_read(16);

			if (TH == 'T')
			{
				printf("\t\t\t\t\t\tT %04x\n", tempreading);

	tempdata = (tempreading * 0.01) - 40.0;
if (!bad)
lasttemp = tempdata;


			}
			if (TH == 'H')
			{
				printf("\t\t\t\t\t\t\t\t\tH %04x\n", tempreading);


            double C1 = -4;
            double C2 = 0.0405;
            double C3 = -2.8E-06;
            double T1 = 0.01;
            double T2 = 0.00008;
            double rh = round(tempreading);
			double rh_lin = ((C3 * rh) * rh) + (C2 * rh) + C1;
            double rh_true = (lasttemp - 25) * (T1 + (T2 * rh)) + rh_lin;
            if (rh_true > 100) { rh_true = 100;}
            if (rh_true < 0.1){ rh_true = 0.1; }
tempdata = rh_true;

			}

temper_write_complex(0x01, 1);

if (bad)
return -99;
			return tempdata;
}

