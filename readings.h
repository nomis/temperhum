#define SHT1X_VER 4
#define SHT1X_VOLTAGE 5000
#define SHT1X_TEMP_RES 14
#define SHT1X_HUM_RES 12

struct sht1x_readings {
	int err;
	double temperature_celsius;
	double relative_humidity;
	double dew_point;
};

struct sht1x_readings sht1x_getreadings(void);
