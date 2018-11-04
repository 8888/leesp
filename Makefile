CC = cc
CFLAGS = -std=c99 -Wall
LFLAGS = -ledit -lm

leesp: include/mpc/mpc.o main.o
	$(CC) $(CFLAGS) $^ $(LFLAGS) -o leesp
