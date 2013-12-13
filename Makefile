CC=gcc

all: target

debug: CC += -DDEBUG
debug: target

target: gpio-loopback
	${CC} gpio-loopback.c -o gpio-loopback

clean:
	rm -rf *.o gpio-loopback
