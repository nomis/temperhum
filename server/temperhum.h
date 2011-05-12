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

struct th_readings {
	double temperature_celsius;
	double relative_humidity;
	double dew_point;
};

struct th_device {
	char name[4096];
	char rrdfile[4096];
	struct th_readings readings;
	struct th_device *next;
};

struct th_socket {
	int fd;
	struct th_socket *next;
};

#define DEFAULT_SERVICE "21576"
