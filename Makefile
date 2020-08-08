CC=gcc
CFLAGS=-I.
LDFLAGS=-lrt -lpthread
OBJS=ringbuf.o
DEBUG=0

ifeq ($(DEBUG),1)
	CFLAGS+=-g # -O0
else
	CFLAGS+=-O2
endif

all:: ringbuf 

ringbuf: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean: 
	rm -f ringbuf *.o *~
