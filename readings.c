#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comms.h"
#include "readings.h"

enum sht1x_type { TEMP, HUMIDITY };

double sht1x_sample(struct sht1x_readings readings, int low_resolution, enum sht1x_type type) {
	unsigned int resp;
	int err;

	if (type == TEMP) {
		err = sht1x_command(SHT1X_ADDR, SHT1X_CMD_M_TEMP);
	} else if (type == HUMIDITY) {
		/* Don't bother reading humidity with temperature */
		if (isnan(readings.temperature_celsius))
			return NAN;

		err = sht1x_command(SHT1X_ADDR, SHT1X_CMD_M_RH);
	} else {
		return NAN;
	}

	if (err)
		return NAN;

	resp = sht1x_read(2);
//	if ((resp & 0xFF000000) != 0)
//		return NAN;

	if (type == TEMP) {
#if SHT1X_VOLTAGE == 5000
		double D1 = -40.1;
#elif SHT1X_VOLTAGE == 4500
		double D1 = -39.8;
#elif SHT1X_VOLTAGE == 3500
		double D1 = -39.7;
#elif SHT1X_VOLTAGE == 3000
		double D1 = -39.6;
#elif SHT1X_VOLTAGE == 2500
		double D1 = -39.4;
#else
		double D1 = NAN;
#		error "Unknown SHT1X voltage"
#endif
		double D2;

		if (low_resolution)
			D2 = 0.04;
		else
			D2 = 0.01;

		return D1 + D2 * (resp & 0x0000FFFF);
	} else if (type == HUMIDITY) {
		double C1, C2, C3;
		double T1, T2;
		double rh, rh_lin, rh_true;

		if (low_resolution) {
			C1 = -2.0468;
			C2 = 0.5872;
			C3 = -4.0845E-04;
			T1 = 0.01;
			T2 = 0.00128;
		} else {
			C1 = -2.0468;
			C2 = 0.0367;
			C3 = -1.5955E-06;
			T1 = 0.01;
			T2 = 0.00008;
		}

		rh = (resp & 0x0000FFFF);
		rh_lin = C1 + (C2 * rh) + (C3 * rh * rh);
		rh_true = (readings.temperature_celsius - 25.0) * (T1 + (T2 * rh)) + rh_lin;

		if (rh_true > 99.0)
			rh_true = 100.0;
		if (rh_true < 0.1)
			rh_true = 0.1;

		return rh_true;
	} else {
		return NAN;
	}
}

struct sht1x_readings sht1x_getreadings(int low_resolution) {
	struct sht1x_readings readings = {
		.temperature_celsius = NAN,
		.relative_humidity = NAN,
		.dew_point = NAN
	};

	readings.temperature_celsius = sht1x_sample(readings, low_resolution, TEMP);
	readings.relative_humidity = sht1x_sample(readings, low_resolution, HUMIDITY);

	/* Calculate de point */
	if (!isnan(readings.temperature_celsius) && !isnan(readings.relative_humidity)) {
		double TN, M;
		double tmp;
		if (readings.temperature_celsius > 0) {
			TN = 243.12;
			M = 17.62;
		} else {
			TN = 272.62;
			M = 22.46;
		}
		tmp = log(readings.relative_humidity / 100.0)
			+ ((M * readings.temperature_celsius) / (TN + readings.temperature_celsius));
		readings.dew_point = (TN * tmp) / (M - tmp);
	}

	return readings;
}