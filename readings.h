#define SHT1X_VOLTAGE 5000

struct sht1x_readings {
	int err;
	double temperature_celsius;
	double relative_humidity;
	double dew_point;
};

struct sht1x_readings sht1x_getreadings(int low_resolution);
