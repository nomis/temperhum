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

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <rrd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "comms.h"
#include "readings.h"

void temperhum_rrd_open(char *fname) {
	struct stat buf;

	if (stat(fname, &buf) != 0) {
		const char *args[7] = {
			"DS:tc:GAUGE:60:U:U",
			"DS:rh:GAUGE:60:U:U",
			"DS:dp:GAUGE:60:U:U",
			"RRA:AVERAGE:0:1:604800",
			"RRA:AVERAGE:0.5:5:6289920",
			"RRA:AVERAGE:0.5:60:5241600",
			"RRA:AVERAGE:0.5:600:5241600"
		};
		rrd_clear_error();
		if (rrd_create_r(fname, 1, time(NULL) - 1, 7, args) != 0) {
			fprintf(stderr, "%s: %s\n", fname, rrd_get_error());
			exit(EXIT_FAILURE);
		}
	}
}

void temperhum_rrd_update(char *fname, time_t tv_sec, struct sht1x_readings readings) {
	char update[4096];
	char buf[512];
	const char *args[1];

	args[0] = update;
	snprintf(update, 4096, "%llu", (unsigned long long)tv_sec);

	if (isnan(readings.temperature_celsius))
		strcat(update, ":U");
	else {
		snprintf(buf, 512, ":%.2f", readings.temperature_celsius);
		strcat(update, buf);
	}

	if (isnan(readings.relative_humidity))
		strcat(update, ":U");
	else {
		snprintf(buf, 512, ":%.2f", readings.relative_humidity);
		strcat(update, buf);
	}

	if (isnan(readings.dew_point))
		strcat(update, ":U");
	else {
		snprintf(buf, 512, ":%.2f", readings.dew_point);
		strcat(update, buf);
	}

	rrd_clear_error();
	if (rrd_update_r(fname, "tc:rh:dp", 1, args) != 0)
		fprintf(stderr, "%s: (%s) %s\n", fname, args[0], rrd_get_error());
}

int main(int argc, char *argv[]) {
	char rrdfile[4096];
	struct sht1x_status status;
	int err;
	(void)argc;

	if (argc != 2) {
		printf("Usage: %s 1-2.3.4\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	snprintf(rrdfile, 4096, "%s.rrd", argv[1]);

	sht1x_open(argv[1]);
	temperhum_rrd_open(rrdfile);

	err = sht1x_device_reset();
	if (err) {
		fprintf(stderr, "Error resetting device\n");
		exit(EXIT_FAILURE);
	}

	status = sht1x_read_status();
	if (!status.valid) {
		fprintf(stderr, "Status read failed\n");
		sht1x_close();
		exit(EXIT_FAILURE);
	} else {
		printf("; Status {"
			"Battery: %s, "
			"Heater: %s, "
			"Reload: %s, "
			"Resolution: %s }\n",
			status.low_battery == 0 ? "OK" : "LOW",
			status.heater == 0 ? "OFF" : "ON",
			status.no_reload == 0 ? "YES" : "NO",
			status.low_resolution == 0 ? "HIGH" : "LOW");

		if (status.low_resolution || status.heater) {
			status.low_resolution = 0;
			status.heater = 0;

			if (sht1x_write_status(status)) {
				fprintf(stderr, "Status write failed\n");
				sht1x_close();
				exit(EXIT_FAILURE);
			}
		}
	}

	while(1) {
		struct sht1x_readings readings = sht1x_getreadings(status.low_resolution);
		struct timespec tp;
		clock_gettime(CLOCK_REALTIME, &tp);
		printf("%lu.%09lu; T %6.2f℃ / RH %6.2f%% / DP %6.2f℃\n", tp.tv_sec, tp.tv_nsec, readings.temperature_celsius, readings.relative_humidity, readings.dew_point);
		temperhum_rrd_update(rrdfile, tp.tv_sec, readings);
	}

	sht1x_close();
	exit(EXIT_SUCCESS);
}
