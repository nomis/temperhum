/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define SHT1X_VOLTAGE 5000

struct sht1x_readings {
	int err;
	double temperature_celsius;
	double relative_humidity;
	double dew_point;
};

struct sht1x_readings sht1x_getreadings(struct sht1x_device *dev, int low_resolution);
