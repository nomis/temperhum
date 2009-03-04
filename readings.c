#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comms.h"
#include "readings.h"

enum temper_type { TEMP, HUMIDITY };

double temper_sample(struct temper_readings readings, enum temper_type type) {
	int xxx=0, bad=0;
	double tempdata=0;
	int tempreading;

	temper_write_complex(0x02, 1);
	temper_write_complex(0x01, 1);

	if (type == TEMP)
		temper_write_simple(0x03, 8);
	else if (type == HUMIDITY)
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

	tempreading = temper_read(16);

	if (type == TEMP) {
		printf("\t\t\t\t\t\tT %04x\n", tempreading);
		tempdata = (tempreading / 100.0) - 40.0;
	} else if (type == HUMIDITY) {
		printf("\t\t\t\t\t\t\t\t\tH %04x\n", tempreading);

		if (isnan(readings.temperature_celsius)) {
			tempdata = FP_NAN;
		} else {
			double C1 = -4;
			double C2 = 0.0405;
			double C3 = -2.8E-06;
			double T1 = 0.01;
			double T2 = 0.00008;
			double rh = round(tempreading);
			double rh_lin = ((C3 * rh) * rh) + (C2 * rh) + C1;
			double rh_true = (readings.temperature_celsius - 25) * (T1 + (T2 * rh)) + rh_lin;
			if (rh_true > 100) { rh_true = 100;}
			if (rh_true < 0.1){ rh_true = 0.1; }
			tempdata = rh_true;
		}
	}

	temper_write_complex(0x01, 1);

	if (bad)
		return FP_NAN;
	return tempdata;
}

struct temper_readings temper_getreadings(void) {
	struct temper_readings readings = {
		.temperature_celsius = FP_NAN,
		.relative_humidity = FP_NAN
	};

	readings.temperature_celsius = temper_sample(readings, TEMP);
	readings.relative_humidity = temper_sample(readings, HUMIDITY);

	return readings;
}
