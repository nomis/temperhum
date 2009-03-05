#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "comms.h"
#include "readings.h"

int main(int argc, char *argv[]) {
	struct sht1x_status status;
	(void)argc;

	sht1x_open(argv[1]);

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
		struct timespec delay;
		struct sht1x_readings readings;

		if (clock_gettime(CLOCK_REALTIME, &delay) != 0) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		if (delay.tv_nsec >= 750000000)
			delay.tv_sec++;
		delay.tv_nsec = 999999999;

		if (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &delay, NULL) != 0) {
			perror("clock_nanosleep");
			exit(EXIT_FAILURE);
		}

		readings = sht1x_getreadings(status.low_resolution);
		printf("; T %3.2f℃ / RH %5.1f%% / DP %3.2f℃\n", readings.temperature_celsius, readings.relative_humidity, readings.dew_point);
	}

	sht1x_close();
	exit(EXIT_SUCCESS);
}
