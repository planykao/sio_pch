# compiler
CC = gcc

# include directory
INCLUDE = ${PWD}/include

# directory
CONFDIR = ${PWD}/conf
LP_CONF_DIR = $(CONFDIR)/loopback
SCRIPTDIR = ${PWD}/script
RELEASEDIR = ${PWD}/release
LP_RELEASE_DIR = $(RELEASEDIR)/loopback
GPIO_RELEASE_DIR = $(RELEASEDIR)/gpio
BP_RELEASE_DIR = $(RELEASEDIR)/bypass
HWMON_RELEASE_DIR = $(RELEASEDIR)/hwmon
WDT_RELEASE_DIR = $(RELEASEDIR)/wdt
TOOLS_DIR	:= tools

# all tools
TOOLS_GPIO = $(TOOLS_DIR)/gpio
TOOLS_LP = $(TOOLS_DIR)/loopback
TOOLS_HWMON = $(TOOLS_DIR)/hwmon
TOOLS_WDT = $(TOOLS_DIR)/wdt
TOOLS_BP = $(TOOLS_DIR)/bypass

# CFLAGS
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

CFLAGS += -I$(INCLUDE) -D_GNU_SOURCE

GIT = $(shell which git > /dev/null 2>&1; echo $$?)

.PHONY: all clean

all:

.PHONY: release
release:
	@mkdir -p $(RELEASEDIR)
	@mkdir -p $(LP_RELEASE_DIR)
	@mkdir -p $(GPIO_RELEASE_DIR)
	@mkdir -p $(BP_RELEASE_DIR)
	@mkdir -p $(HWMON_RELEASE_DIR)
	cp -rf README.USER $(RELEASEDIR)
	cp -rf $(CONFDIR)/* $(RELEASEDIR)
	cp -rf $(SCRIPTDIR)/* $(RELEASEDIR)
	if [ -f $(TOOLS_LP) ]; then mv $(TOOLS_LP) $(LP_RELEASE_DIR); fi
	if [ -f $(TOOLS_GPIO) ]; then mv $(TOOLS_GPIO) $(GPIO_RELEASE_DIR); \
	cp -rf $(LP_CONF_DIR)/* $(GPIO_RELEASE_DIR); fi
	if [ -f $(TOOLS_BP) ]; then mv $(TOOLS_BP) $(BP_RELEASE_DIR); fi
	if [ -f $(TOOLS_HWMON) ]; then mv $(TOOLS_HWMON) $(HWMON_RELEASE_DIR); fi
	if [ -f $(TOOLS_WDT) ]; then mv $(TOOLS_WDT) $(WDT_RELEASE_DIR); fi

.PHONY: changelog
changelog:
ifeq ($(GIT), 1)
	@echo "git not found, please install git first."
else
	git log > $(CHANGELOG)
endif

# import tools/Module.mk
include tools/Module.mk

