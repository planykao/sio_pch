# compiler
CC = gcc

# include directory
INCLUDE = ${PWD}/

# directory
CONFDIR = ${PWD}/conf
SCRIPTDIR = ${PWD}/script
RELEASEDIR = ${PWD}/release
LP_RELEASE_DIR = $(RELEASEDIR)/loopback
BP_RELEASE_DIR = $(RELEASEDIR)/bypass
HWMON_RELEASE_DIR = $(RELEASEDIR)/hwmon

# files
GPIO = gpio-loopback
HWMON = hwmon
BYPASS = bypass
LIBPCH = libpch.c
LIBSIO = libsio.c
BIN = $(GPIO) $(HWMON) $(BYPASS)
CHANGELOG = Changelog

# objects
GPIO_OBJS = libpch.o libsio.o
HWMON_OBJS = libsio.o
BP_OBJS = libsio.o

GIT = $(shell which git > /dev/null 2>&1; echo $$?)

# CFLAGS
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

CFLAGS += -I$(INCLUDE)

# targets
all: gpio hwmon bypass changelog release

gpio: $(GPIO_OBJS) $(GPIO).c
	$(CC) $(CFLAGS) $(GPIO_OBJS) $(GPIO).c -o $(GPIO)

hwmon: $(HWMON_OBJS) $(HWMON).c
	$(CC) $(CFLAGS) $(HWMON_OBJS) $(HWMON).c -o $(HWMON)

bypass: $(BP_OBJS) $(BYPASS).c
	$(CC) $(CFLAGS) $(BP_OBJS) $(BYPASS).c -o $(BYPASS)

libpch.o: $(LIBPCH)
	$(CC) $(CFLAGS) -c $(LIBPCH)

libsio.o: $(LIBSIO)
	$(CC) $(CFLAGS) -c $(LIBSIO)

.PHONY: changelog
changelog:
ifeq ($(GIT), 1)
	@echo "git not found, please install git first."
else
	git log > $(CHANGELOG)
endif

.PHONY: release	
release:
	mkdir -p $(RELEASEDIR)
	cp -rf $(CONFDIR)/* $(RELEASEDIR)
	cp -rf $(SCRIPTDIR)/* $(RELEASEDIR)
	if [ -f $(GPIO) ]; then cp -rf $(GPIO) $(LP_RELEASE_DIR); fi
	if [ -f $(BYPASS) ]; then cp -rf $(BYPASS) $(BP_RELEASE_DIR); fi
	if [ -f $(HWMON) ]; then cp -rf $(HWMON) $(HWMON_RELEASE_DIR); fi

.PHONY: clean
clean:
	rm -rf *.o $(BIN) $(RELEASEDIR)/* $(CHANGELOG)
