# compiler
CC = gcc

# include directory
INCLUDE = ../include

# files
LIBPCH = ../tools/libpch.c
LIBSIO = ../tools/libsio.c
TARGET = s0081_led s0951_nmi sch5027 gpioset
BIN = *.o $(TARGET)
CHANGELOG = Changelog

# objects
OBJS = ../tools/libpch.o ../tools/libsio.o

GIT = $(shell which git > /dev/null 2>&1; echo $$?)

# CFLAGS
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

CFLAGS += -I$(INCLUDE) -D_GNU_SOURCE

# targets
all: $(TARGET)

gpioset: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) gpioset.c -o gpioset

s0081_led: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) s0081_led.c -o s0081_led

s0951_nmi: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) s0951_nmi.c -o s0951_nmi

libpch.o: $(LIBPCH)
	$(CC) $(CFLAGS) -c $(LIBPCH)

libsio.o: $(LIBSIO)
	$(CC) $(CFLAGS) -c $(LIBSIO)

sch5027: libsio.o
	$(CC) $(CFLAGS) libsio.o sch5027.c -o sch5027

.PHONY: clean
clean:
	rm -rf $(BIN) 
