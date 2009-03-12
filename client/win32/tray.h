enum conn_status {
	NOT_CONNECTED,
	CONNECTING,
	CONNECTED
};

struct tray_status {
	enum conn_status conn;
	double temperature_celsius;
	double relative_humidity;
	double dew_point;
};

void update_tray(struct tray_status *status);
