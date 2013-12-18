CC = gcc
INCLUDE = ${PWD}/

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS = -DDEBUG
else
	CFLAGS = -DNDEBUG
endif

all: gpio

gpio: siolib.o pchlib.o
	$(CC) $(CFLAGS) -I$(INCLUDE) siolib.o pchlib.o gpio-loopback.c \
		-o gpio-loopback

hwmon:
	$(CC) $(CFLAGS) -I$(INCLUDE) hwmon.c -o hwmon

pchlib.o: pchlib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c pchlib.c

siolib.o: siolib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c siolib.c

clean:
	rm -rf *.o gpio-loopback
