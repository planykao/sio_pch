CC = gcc
INCLUDE = ${PWD}/

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS = -DDEBUG
else
	CFLAGS = -DNDEBUG
endif

all: gpio

gpio: sio_gpiolib.o pch_gpiolib.o
	$(CC) $(CFLAGS) -I$(INCLUDE) sio_gpiolib.o pch_gpiolib.o gpio-loopback.c \
		-o gpio-loopback

pch_gpiolib.o: pch_gpiolib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c pch_gpiolib.c

sio_gpiolib.o: sio_gpiolib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c sio_gpiolib.c

clean:
	rm -rf *.o gpio-loopback
