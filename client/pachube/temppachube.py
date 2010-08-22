#!/usr/bin/env python

from __future__ import print_function
import eeml
import socket
import sys
import time

sensors = {
	"front_room": "front-bedroom",
	"rear_room": "rear-bedroom",
	"comp": "computers"
}

def process(feed, c, id, card, mux, data):
	global muxes, stats

	_strength = []
	_quality = []
	_signal = []

	for line in data.split("\n"):
		match = stats.match(line)
		if not match:
			continue
		match = match.groupdict()

		strength = float(match["signal"]) / 655.36
		quality = float(match["snr"]) / 655.36
		signal = float(match["signal"]) / 655.36 * (float(match["snr"]) / 65536.0)

		c.execute("INSERT INTO stats (card, mux, strength, quality) VALUES(%(card)s, %(mux)s, %(strength)s, %(quality)s)", { "card": card, "mux": mux, "strength": strength, "quality": quality })

		_strength.append(strength)
		_quality.append(quality)
		_signal.append(signal)

	if len(_strength) > 0:
		feed.update([eeml.Data("{0}.strength".format(id), "{0:.4f}".format(float(sum(_strength)) / len(_strength)))])
	if len(_quality) > 0:
		feed.update([eeml.Data("{0}.quality".format(id), "{0:.4f}".format(float(sum(_quality)) / len(_quality)))])
	if len(_signal) > 0:
		feed.update([eeml.Data("{0}.signal".format(id), "{0:.4f}".format(float(sum(_signal)) / len(_signal)))])

with open("key") as f:
	key = f.read().rstrip()

if len(sys.argv) < 2:
	print("Usage: temppachube.py <id> [host] [port]")
	sys.exit(1)

id = sys.argv[1]
host = sys.argv[2] if len(sys.argv) > 2 else "localhost"
port = sys.argv[3] if len(sys.argv) > 3 else "temperhum"

s = socket.create_connection((host, port))
f = s.makefile()

feed = None
sensor = None
last = 0
all = 0
while True:
	if feed is None:
		feed = eeml.Pachube("/api/feeds/{0}.xml".format(id), key)

	line = f.readline().rstrip().split(" ")
	if line[0] == "SENSD":
		sensor = line[1]
		if all < 2:
			all = all + 1
	elif line[0] == "SENSR":
		sensor = line[1]
	elif line[0] == "SENSF":
		sensor = None
	elif all == 2 and line[0] == "TEMPC" and sensor in sensors:
		feed.update([eeml.Data("temp.{0}".format(sensors[sensor]), line[1])])
		now = time.time()
		if now - last >= 59:
			last = now
			feed.put()
			feed = None
