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

void temperhum_rrd_create(char *fname) {
	struct stat buf;

	if (stat(fname, &buf) != 0) {
		const char *args[21] = {
			"DS:tc:GAUGE:60:U:U",
			"DS:rh:GAUGE:60:U:U",
			"DS:dp:GAUGE:60:U:U",

			"RRA:HWPREDICT:86400:0.75:0.75:86400", /* daily */
			"RRA:HWPREDICT:86400:0.75:0.75:31449600", /* yearly */

			"RRA:AVERAGE:0:1:604800",        /* avg  1s for  1w */

			"RRA:MIN:0.5:30:1051920",        /* min             */
			"RRA:AVERAGE:0.5:30:1051920",    /* avg 30s for  1y */
			"RRA:MAX:0.5:30:1051920",        /* max             */

			"RRA:MIN:0.5:600:1051920",       /* min             */
			"RRA:AVERAGE:0.5:600:1051920",   /* avg 10m for 20y */
			"RRA:MAX:0.5:600:1051920",       /* max             */

			"RRA:MIN:0.5:3600:175320",       /* min             */
			"RRA:AVERAGE:0.5:3600:175320",   /* avg  1h for 20y */
			"RRA:MAX:0.5:3600:175320",       /* max             */

			"RRA:MIN:0.5:21600:29220",       /* min             */
			"RRA:AVERAGE:0.5:21600:29220",   /* avg  6h for 20y */
			"RRA:MAX:0.5:21600:29220",       /* max             */

			"RRA:MIN:0.5:86400:7305",        /* min             */
			"RRA:AVERAGE:0.5:86400:7305",    /* avg  1h for 20y */
			"RRA:MAX:0.5:86400:7305",        /* max             */
		};
		rrd_clear_error();
		if (rrd_create_r(fname, 1, time(NULL) - 1, 21, args) != 0) {
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
	struct sht1x_device *devices = NULL;
	struct sht1x_device *current = NULL;
	int err, status, i;

	if (argc < 2) {
		printf("Usage: %s 1-2.3.4 [1-2.3.5] [2-6.1]\n", argv[0]);
		status = EXIT_FAILURE;
		goto done;
	}

	for (i = 1; i < argc; i++) {
		current = malloc(sizeof(struct sht1x_device));
		if (current == NULL) {
			status = EXIT_FAILURE;
			goto closeall;
		}

		current->next = devices;
		devices = current;

		strncpy(current->name, argv[i], sizeof(current->name));
		snprintf(current->rrdfile, sizeof(current->rrdfile), "%s.rrd", argv[i]);

		sht1x_open(current);
		temperhum_rrd_create(current->rrdfile);

		err = sht1x_device_reset(current);
		if (err) {
			fprintf(stderr, "Error resetting device %s\n", current->name);
			status = EXIT_FAILURE;
			goto closeall;
		}

		current->status = sht1x_read_status(current);
		if (!current->status.valid) {
			fprintf(stderr, "Status read failed for %s\n", current->name);
			status = EXIT_FAILURE;
			goto closeall;
		} else {
			printf("%s; Status {"
				"Battery: %s, "
				"Heater: %s, "
				"Reload: %s, "
				"Resolution: %s }\n",
				current->name,
				current->status.low_battery == 0 ? "OK" : "LOW",
				current->status.heater == 0 ? "OFF" : "ON",
				current->status.no_reload == 0 ? "YES" : "NO",
				current->status.low_resolution == 0 ? "HIGH" : "LOW");

			if (current->status.low_resolution || current->status.heater) {
				current->status.low_resolution = 0;
				current->status.heater = 0;

				if (sht1x_write_status(current, current->status)) {
					fprintf(stderr, "Status write failed for %s\n", current->name);
					status = EXIT_FAILURE;
					goto closeall;
				}
			}
		}
	}

	while(1) {
		struct sht1x_readings readings;
		struct timespec tp;

		if (current == NULL)
			current = devices;

		readings = sht1x_getreadings(current, current->status.low_resolution);
		clock_gettime(CLOCK_REALTIME, &tp);
		printf("%lu.%09lu/%10s; T %6.2lf℃ / RH %6.2lf%% / DP %6.2lf℃\n", tp.tv_sec, tp.tv_nsec, current->name, readings.temperature_celsius, readings.relative_humidity, readings.dew_point);
		temperhum_rrd_update(current->rrdfile, tp.tv_sec, readings);

		current = current->next;
	}
	status = EXIT_SUCCESS;

closeall:
	while (devices != NULL) {
		current = devices;
		sht1x_close(current);
		devices = current->next;
		free(current);
	}

done:
	exit(status);
}
