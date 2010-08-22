#!/usr/bin/env python

from __future__ import print_function
import eeml
import socket
import sys
import time

with open("key") as f:
	key = f.read().rstrip()

sensors = {}

with open("config") as f:
	while True:
		line = f.readline()
		if line is None or line == "":
			break
		(name, value) = line.rstrip().split(" ")
		sensors[name] = value

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
