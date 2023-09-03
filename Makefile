
CFLAGS = -g

all: ssplit plotter

plotter: splot.c
	$(CC) $(CFLAGS) splot.c -o splot -lraylib

ssplit: ssplit.c
	$(CC) $(CFLAGS) ssplit.c -o ssplit 


