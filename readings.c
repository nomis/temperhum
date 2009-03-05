#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comms.h"
#include "readings.h"

enum sht1x_type { TEMP, HUMIDITY };

double sht1x_sample(struct sht1x_readings readings, enum sht1x_type type) {
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
	if ((resp & 0xFF000000) != 0)
		return NAN;

	if (type == TEMP) {
#if SHT1X_VER >= 3
#	if SHT1X_VOLTAGE == 5000
		double D1 = -40.1;
#	elif SHT1X_VOLTAGE == 4500
		double D1 = -39.8;
#	elif SHT1X_VOLTAGE == 3500
		double D1 = -39.7;
#	elif SHT1X_VOLTAGE == 3000
		double D1 = -39.6;
#	elif SHT1X_VOLTAGE == 2500
		double D1 = -39.4;
#	else
		double D1 = NAN;
#		error "Unknown SHT1X voltage"
#	endif
#	if SHT1X_TEMP_RES == 14
		double D2 = 0.01;
#	elif SHT1X_TEMP_RES == 12
		double D2 = 0.04;
#	else
		double D2 = NAN;
#		error "Unknown SHT1X temperature resolution"
#	endif
#else
#	error "Unknown SHT1X version"
#endif
		return D1 + D2 * (resp & 0x0000FFFF);
	} else if (type == HUMIDITY) {
#if SHT1X_VER >= 4
#	if SHT1X_HUM_RES == 12
		double C1 = -2.0468;
		double C2 = 0.0367;
		double C3 = -1.5955E-06;
		double T1 = 0.01;
		double T2 = 0.00008;
#	elif SHT1X_HUM_RES == 8
		double C1 = -2.0468;
		double C2 = 0.5872;
		double C3 = -4.0845E-04;
		double T1 = 0.01;
		double T2 = 0.00128;
#	else
		double C1 = NAN;
		double C2 = NAN;
		double C3 = NAN;
		double T1 = NAN;
		double T2 = NAN;
#		error "Unknown SHT1X humidity resolution"
#	endif
#elif SHT1X_VER >= 3
#	if SHT1X_HUM_RES == 12
		double C1 = -4.000;
		double C2 = 0.0405;
		double C3 = -2.8000E-06;
		double T1 = 0.01;
		double T2 = 0.00008;
#	elif SHT1X_HUM_RES == 8
		double C1 = -4.0000;
		double C2 = 0.6480;
		double C3 = -7.2000E-04;
		double T1 = 0.01;
		double T2 = 0.00128;
#	else
		double C1 = NAN;
		double C2 = NAN;
		double C3 = NAN;
		double T1 = NAN;
		double T2 = NAN;
#		error "Unknown SHT1X humidity resolution"
#	endif
#else
		double C1 = NAN;
		double C2 = NAN;
		double C3 = NAN;
		double T1 = NAN;
		double T2 = NAN;
#	error "Unknown SHT1X version"
#endif
		double rh = (resp & 0x0000FFFF);
		double rh_lin = C1 + (C2 * rh) + (C3 * rh * rh);
		double rh_true = (readings.temperature_celsius - 25.0) * (T1 + (T2 * rh)) + rh_lin;
		if (rh_true > 99.0)
			rh_true = 100.0;
		if (rh_true < 0.1)
			rh_true = 0.1;
		return rh_true;
	} else {
		return NAN;
	}
}

struct sht1x_readings sht1x_getreadings(void) {
	struct sht1x_readings readings = {
		.temperature_celsius = NAN,
		.relative_humidity = NAN,
		.dew_point = NAN
	};

	readings.temperature_celsius = sht1x_sample(readings, TEMP);
	readings.relative_humidity = sht1x_sample(readings, HUMIDITY);

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
