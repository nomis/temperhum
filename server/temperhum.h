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
