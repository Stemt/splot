
CFLAGS = -g

all: ssplit splot

splot: splot.c
	$(CC) $(CFLAGS) splot.c -o splot -lraylib

ssplit: ssplit.c
	$(CC) $(CFLAGS) ssplit.c -o ssplit 


