CC = gcc
INCLUDE = ${PWD}/
GPIO_OBJS = pchlib.o siolib.o
HWMON_OBJS = siolib.o pin_list.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS = -DDEBUG
else
	CFLAGS = -DNDEBUG
endif

all: gpio hwmon

gpio: $(GPIO_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(GPIO_OBJS) gpio-loopback.c \
		-o gpio-loopback

hwmon: $(HWMON_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(HWMON_OBJS) hwmon.c -o hwmon

pchlib.o: pchlib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c pchlib.c

siolib.o: siolib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c siolib.c

pin_list.o: pin_list.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c pin_list.c

clean:
	rm -rf *.o gpio-loopback
