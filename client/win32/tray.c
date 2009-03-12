#include "debug.h"
#include "tray.h"

void update_tray(struct tray_status *status) {
	odprintf("tray: conn=%d, temperature_celsius=%f, relative_humidity=%f, dew_point=%f",
		status->conn, status->temperature_celsius, status->relative_humidity, status->dew_point);
}
