CC=gcc

all: gpio

debug: CC += -DDEBUG
debug: target

gpio: siolib.o gpio-loopback.h
	${CC} siolib.o gpio-loopback.c -o gpio-loopback -I${PWD}

siolib.o: gpio-loopback.h siolib.c siolib.h
	${CC} siolib.c -c -I${PWD}/

clean:
	rm -rf *.o gpio-loopback
