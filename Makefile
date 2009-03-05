all:
	gcc -Wall -Wextra -lm -lrt -O2 -D_POSIX_C_SOURCE=200112L -D_ISOC99_SOURCE comms.c readings.c temperhum.c -o temperhum

