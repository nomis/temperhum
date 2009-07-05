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
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <rrd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "temperhum.h"

volatile sig_atomic_t stop = 0;

void th_stop(int sig) {
	(void)sig;
	stop = 1;
}

static int th_send(int fd, char *msg, int flags) {
	int len = strlen(msg);
	int ret = send(fd, msg, len, MSG_NOSIGNAL|flags);
	return (ret != len);
}

int main(int argc, char *argv[]) {
	struct th_device *devices = NULL;
	struct th_device *current_d = NULL;
	struct th_socket *clients = NULL;
	struct th_socket *current_c = NULL;
	struct th_socket *servers = NULL;
	struct th_socket *current_s = NULL;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *cur;
	struct timespec last;
	int status, ret, i;

	if (argc < 2) {
		printf("Usage: %s 1-2.3.4 [1-2.3.5] [2-6.1]\n", argv[0]);
		status = EXIT_FAILURE;
		goto done;
	}

	for (i = 1; i < argc; i++) {
		current_d = malloc(sizeof(struct th_device));
		if (current_d == NULL) {
			status = EXIT_FAILURE;
			goto freeall;
		}

		current_d->next = devices;
		devices = current_d;

		strncpy(current_d->name, argv[i], sizeof(current_d->name));
		snprintf(current_d->rrdfile, sizeof(current_d->rrdfile), "%s.rrd", argv[i]);
	}

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_addrlen = 0;
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;

	ret = getaddrinfo(NULL, DEFAULT_SERVICE, &hints, &res);
	if (ret != 0) {
		perror("getaddrinfo");
		status = EXIT_FAILURE;
		goto freeall;
	}

	for (cur = res; cur != NULL; cur = cur->ai_next) {
		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
		int one = 1;
		int flags, l;

		ret = getnameinfo(cur->ai_addr, cur->ai_addrlen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST|NI_NUMERICSERV);
		if (ret != 0) {
			hbuf[0] = 0;
			sbuf[0] = 0;
		}

		l = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
		if (l == -1) {
			perror("socket");
			goto listen_failed;
		}

		if (cur->ai_family == AF_INET6) {
			ret = setsockopt(l, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof(one));
			if (ret != 0) {
				perror("setsockopt");
				close(l);
				goto listen_failed;
			}
		}

		ret = setsockopt(l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
		if (ret != 0) {
			perror("setsockopt");
			close(l);
			goto listen_failed;
		}

		ret = bind(l, cur->ai_addr, cur->ai_addrlen);
		if (ret != 0) {
			perror("bind");
			close(l);
			goto listen_failed;
		}

		ret = listen(l, 10);
		if (ret != 0) {
			perror("listen");
			close(l);
			goto listen_failed;
		}

		flags = fcntl(l, F_GETFL);
		flags |= O_NONBLOCK;
		ret = fcntl(l, F_SETFL, flags);
		if (ret != 0) {
			perror("fcntl");
			close(l);
			goto listen_failed;
		}

		current_s = malloc(sizeof(struct th_socket));
		if (current_s == NULL) {
			status = EXIT_FAILURE;
			goto closeall;
		}

		current_s->next = servers;
		servers = current_s;

		current_s->fd = l;

		if (hbuf[0] != 0 && sbuf[0] != 0)
			fprintf(stderr, "Listening on node \"%s\" service \"%s\"\n", hbuf, sbuf);
		continue;

listen_failed:
		if (hbuf[0] != 0 && sbuf[0] != 0)
			fprintf(stderr, "Unable to listen node \"%s\" service \"%s\"\n", hbuf, sbuf);
	}

	if (servers == NULL) {
		fprintf(stderr, "Unable to bind to service\n");
		status = EXIT_FAILURE;
		goto freeres;
	}

	freeaddrinfo(res);
	res = NULL;

	last.tv_sec = 0;
	last.tv_nsec = 0;

	signal(SIGINT, th_stop);
	signal(SIGTERM, th_stop);

	while(!stop) {
		fd_set r, w, e;
		int max, s;
		struct timeval timeout;
		struct timespec now;
		int run = 0;

		ret = clock_gettime(CLOCK_REALTIME, &now);
		if (ret != 0) {
			perror("clock_gettime");
			status = EXIT_FAILURE;
			goto closeall;
		}

		if (now.tv_sec < last.tv_sec
				|| (now.tv_sec == last.tv_sec && now.tv_nsec < last.tv_nsec)
				|| (now.tv_sec - last.tv_sec > 1)
				|| (now.tv_sec - last.tv_sec == 1 && (now.tv_nsec >= last.tv_nsec)))
			run = 1;

		if (run)
			last.tv_sec = now.tv_sec;

		/* Broadcast to clients */
		if (clients != NULL && run) {
			struct timeval immediate;
			struct th_socket *last_c;
			time_t rrd_start = now.tv_sec - 6;
			time_t rrd_end = now.tv_sec - 1;
			unsigned long rrd_step = 1;
			unsigned long ds_cnt;
			char **ds_namv = NULL;
			rrd_value_t *rrd_data = NULL;
			char buf[128];

			buf[sizeof(buf)/sizeof(char) - 1] = 0;

			if (current_d == NULL)
				current_d = devices;

			rrd_clear_error();
			if (rrd_fetch_r(current_d->rrdfile, "AVERAGE", &rrd_start, &rrd_end, &rrd_step, &ds_cnt, &ds_namv, &rrd_data) != 0) {
				fprintf(stderr, "%s: %s\n", current_d->name, rrd_get_error());
				current_d->readings.temperature_celsius = NAN;
				current_d->readings.relative_humidity = NAN;
				current_d->readings.dew_point = NAN;
			} else {
				time_t rrd_pos;
				unsigned int j = 0;
				unsigned int k = 0;
				int tc = -1;
				int rh = -1;
				int dp = -1;
				unsigned int tc_c = 0;
				unsigned int rh_c = 0;
				unsigned int dp_c = 0;

				for (k = 0; k < ds_cnt; k++) {
					if (tc == -1 && !strcmp(ds_namv[k], "tc")) {
						tc = k;
						current_d->readings.temperature_celsius = 0;
					} else if (rh == -1 && !strcmp(ds_namv[k], "rh")) {
						rh = k;
						current_d->readings.relative_humidity = 0;
					} else if (dp == -1 && !strcmp(ds_namv[k], "dp")) {
						dp = k;
						current_d->readings.dew_point = 0;
					}
				}

#ifdef DEBUG
				printf("rrd for %s\n", current_d->rrdfile);
				printf("start=%ld end=%ld step=%lu\n", rrd_start, rrd_end, rrd_step);
#endif
				for (rrd_pos = rrd_start; rrd_pos < rrd_end; rrd_pos += rrd_step, j++) {
#ifdef DEBUG
					printf("at %ld:", rrd_pos);
					for (k = 0; k < ds_cnt; k++)
						printf(" %s=%.2lf", ds_namv[i], rrd_data[j * ds_cnt + k]);
					printf("\n");
#endif

					if (tc != -1 && !isnan(rrd_data[j * ds_cnt + tc])) {
						current_d->readings.temperature_celsius += rrd_data[j * ds_cnt + tc];
						tc_c++;
					}

					if (rh != -1 && !isnan(rrd_data[j * ds_cnt + rh])) {
						current_d->readings.relative_humidity += rrd_data[j * ds_cnt + rh];
						rh_c++;
					}

					if (dp != -1 && !isnan(rrd_data[j * ds_cnt + dp])) {
						current_d->readings.dew_point += rrd_data[j * ds_cnt + dp];
						dp_c++;
					}
				}

				if (tc_c == 0)
					current_d->readings.temperature_celsius = NAN;
				else
					current_d->readings.temperature_celsius /= tc_c;

				if (rh_c == 0)
					current_d->readings.relative_humidity = NAN;
				else
					current_d->readings.relative_humidity /= rh_c;

				if (dp_c == 0)
					current_d->readings.dew_point = NAN;
				else
					current_d->readings.dew_point /= dp_c;

				for (k = 0; k < ds_cnt; k++)
					free(ds_namv[k]);
				free(ds_namv);
				free(rrd_data);
			}

			FD_ZERO(&r);
			FD_ZERO(&w);
			FD_ZERO(&e);
			max = -1;

			current_c = clients;
			while (current_c != NULL) {
				FD_SET(current_c->fd, &w);
				FD_SET(current_c->fd, &e);
				if (current_c->fd > max)
					max = current_c->fd;
				current_c = current_c->next;
			}

			immediate.tv_sec = 0;
			immediate.tv_usec = 0;

			ret = select(max + 1, &r, &w, &e, &immediate);
			if (ret == -1) {
				perror("select");
				status = EXIT_FAILURE;
				goto closeall;
			}

			last_c = current_c = clients;
			while (current_c != NULL) {
				struct th_socket *next = current_c->next;

				if (FD_ISSET(current_c->fd, &w)) {
					ret = 0;

					snprintf(buf, sizeof(buf), "SENSR %s\n", current_d->name);
					buf[sizeof(buf)/sizeof(char) - 2] = '\n';
					if (current_d->next == NULL)
						buf[4] = 'D';
					ret |= th_send(current_c->fd, buf, MSG_MORE);

					snprintf(buf, sizeof(buf), "TEMPC %.2lf\n", current_d->readings.temperature_celsius);
					buf[sizeof(buf)/sizeof(char) - 2] = '\n';
					ret |= th_send(current_c->fd, buf, MSG_MORE);

					snprintf(buf, sizeof(buf), "RHUM%% %.2lf\n", current_d->readings.relative_humidity);
					buf[sizeof(buf)/sizeof(char) - 2] = '\n';
					ret |= th_send(current_c->fd, buf, MSG_MORE);

					snprintf(buf, sizeof(buf), "DEWPC %.2lf\n", current_d->readings.dew_point);
					buf[sizeof(buf)/sizeof(char) - 2] = '\n';
					ret |= th_send(current_c->fd, buf, MSG_MORE);

					ret |= th_send(current_c->fd, "SENSF\n", 0);

					last_c = current_c;
					current_c = next;
					continue;
				}

				if (current_c == clients)
					last_c = clients = next;
				else
					last_c->next = next;

				close(current_c->fd);
				free(current_c);

				current_c = next;
			}

			current_d = current_d->next;
		}

		/* Accept new connections */
		FD_ZERO(&r);
		FD_ZERO(&w);
		FD_ZERO(&e);
		max = -1;

		current_s = servers;
		while (current_s != NULL) {
			FD_SET(current_s->fd, &r);
			FD_SET(current_s->fd, &e);
			if (current_s->fd > max)
				max = current_s->fd;
			current_s = current_s->next;
		}

		ret = clock_gettime(CLOCK_REALTIME, &now);
		if (ret != 0) {
			perror("clock_gettime");
			status = EXIT_FAILURE;
			goto closeall;
		}

		if (clients != NULL) {
			timeout.tv_sec = 0;
			if (now.tv_sec < last.tv_sec || (now.tv_sec == last.tv_sec && now.tv_nsec < last.tv_nsec) || (now.tv_sec - last.tv_sec) > 1) {
				timeout.tv_usec = 0;
			} else if (now.tv_sec - last.tv_sec == 0) {
				timeout.tv_usec = (1000000000 - (now.tv_nsec - last.tv_nsec))/1000;
			} else if (now.tv_sec - last.tv_sec == 1) {
				timeout.tv_usec = (1000000000 - ((now.tv_nsec + 1000000000) - last.tv_nsec))/1000;
				if (timeout.tv_usec < 0)
					timeout.tv_usec = 0;
			} else {
				timeout.tv_usec = 0;
			}

			ret = select(max + 1, &r, &w, &e, &timeout);
		} else {
			ret = select(max + 1, &r, &w, &e, NULL);
		}

		if (ret == -1) {
			perror("select");
			status = EXIT_FAILURE;
			goto closeall;
		}

		if (ret == 0)
			current_s = NULL;
		else
			current_s = servers;

		while (current_s != NULL) {
			if (FD_ISSET(current_s->fd, &r) || FD_ISSET(current_s->fd, &e)) {
				do {
					s = accept(current_s->fd, NULL, NULL);
					if (s != -1) {
						int flags = fcntl(s, F_GETFL);
						flags |= O_NONBLOCK;
						ret = fcntl(s, F_SETFL, flags);
						if (ret != 0) {
							perror("fcntl");
							close(s);
							s = -1;
						}
					}

					if (s != -1) {
						current_c = malloc(sizeof(struct th_socket));
						if (current_c == NULL) {
							status = EXIT_FAILURE;
							goto closeall;
						}

						current_c->next = clients;
						clients = current_c;

						current_c->fd = s;
					}
				} while (s != -1);
			}

			current_s = current_s->next;
		}
	}
	status = EXIT_SUCCESS;

closeall:
	while (servers != NULL) {
		current_s = servers;
		servers = current_s->next;
		close(current_s->fd);
		free(current_s);
	}

	while (clients != NULL) {
		current_c = clients;
		clients = current_c->next;
		close(current_c->fd);
		free(current_c);
	}

freeres:
	if (res != NULL)
		freeaddrinfo(res);

freeall:
	while (devices != NULL) {
		current_d = devices;
		devices = current_d->next;
		free(current_d);
	}

done:
	exit(status);
}
