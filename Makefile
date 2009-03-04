all:
	gcc -Wall -lm -O2 -D_POSIX_C_SOURCE=199309L -D_ISOC99_SOURCE comms.c hum.c temper.c -o temper

