CC=gcc
CFLAGS=-Wall
OBJS=lightGpio.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJS)

clean:
	rm -f *.o
