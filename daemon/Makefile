CC := gcc
CPP := g++

COMM_FLAGS := -Wall -g -O0

DEBUG_FLAGS = \
	-DDEBUG \
	-DUSE_LOG_ONCE \
#	-DMALLOC_DEBUG_LEVEL=1 \
#	-DMALLOC_DEBUG_LEVEL=2 \
#	-DDEB_PRINTBUF \
#	-DPARSE_DEBUG_ON \
#	-DTHREAD_SAMPLING_DEBUG \
#	-DTHREAD_REPLAY_DEBUG \
#	-DNOLOGI=1

INCLUDE := \
	-I/usr/include \
	-I/usr/include/system \
	-I/usr/include/capi-system-info \
	-I/usr/include/capi-system-runtime-info \
	-I/usr/include/vconf \
	-I/usr/include/ecore-1 \
	-I/usr/include/eina-1 \
	-I/usr/include/eina-1/eina

FLAGS := \
	$(COMM_FLAGS) \
	$(INCLUDE) \
	$(DEBUG_FLAGS)

ifeq (arm, $(findstring arm, $(shell uname -sm)))
	FLAGS += -DDEVICE_ONLY
endif


# compiler flags
CFLAGS := $(FLAGS)
CPPFLAGS := $(FLAGS)

# linker flags
LDFLAGS := \
	-lpthread \
	-lvconf \
	-lsmack \
	-lecore \
	-lcapi-system-info \
	-lwebsockets \
	-ljson

SRC_C := \
	buffer.c \
	da_data.c \
	da_debug.c \
	da_protocol.c \
	da_protocol_inst.c \
	da_inst.c \
	daemon.c \
	debug.c \
	elf.c \
	ioctl_commands.c \
	main.c \
	sys_stat.c \
	threads.c \
	transfer_thread.c \
	utils.c \
	da_protocol_check.c \
	md5.c \
	input_events.c \
	device_vconf.c \
	device_system_info.c \
	device_camera.c \
	smack.c \
	malloc_debug.c \
	target.c \
	thread.c \
	wsi.c

SRC_CPP :=

OBJS_C := $(SRC_C:.c=.o)
OBJS_CPP := $(SRC_CPP:.cpp=.o)
OBJS := $(OBJS_C) $(OBJS_CPP)

TARGET = da_manager
DASCRIPT = da_command
START_SH = ../scripts/start.sh
STOP_SH = ../scripts/stop.sh
PRELOAD_SCRIPT = ../scripts/gen_preload_header.sh

all: debug

release: CFLAGS += -DNOLOGI=1
release: CPPFLAGS += -DNOLOGI=1
release: debug

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(CPP) $(CPPFLAGS) -c -o $@ $<


debug: $(OBJS)
	$(CPP) $(LDFLAGS) $(OBJS) -o $(TARGET)


install: BINDIR = $(DESTDIR)/usr/bin
install: OPTDIR = $(DESTDIR)/opt/swap/sdk
install: $(TARGET)
	mkdir -p $(BINDIR)
	mkdir -p $(OPTDIR)
	install $(TARGET) -t $(BINDIR)
	bash $(PRELOAD_SCRIPT) $(START_SH)
	install -m 755 $(START_SH) $(STOP_SH) -t $(OPTDIR)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all debug release clean install
