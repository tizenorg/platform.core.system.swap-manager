CC :=gcc
DEBUG_CPPFLAGS = 				\
		-DDEBUG					\
		-DUSE_LOG_ONCE			\
#		-DDEB_PRINTBUF			\#
#		-DPARSE_DEBUG_ON		\#
#		-DTHREAD_SAMPLING_DEBUG	\#
#		-DTHREAD_REPLAY_DEBUG	#

CPPFLAGS := 	-I/usr/include		\
		-I/usr/include/system 	\
		-I/usr/include/telephony\
		-I/usr/include/vconf	\
		$(DEBUG_CPPFLAGS)
ifeq (arm, $(findstring arm, $(shell uname -sm)))
	CPPFLAGS += -DDEVICE_ONLY
endif

COMMON_LIBS := -lglib-2.0 -lpthread
TARGET_LIBS := $(COMMON_LIBS) -lvconf -lsmack -lcapi-system-info -lecore -leina -lpthread
CFLAGS := -Wall -pthread -g -O0 -I/usr/include/ecore-1 -I/usr/include/eina-1 -I/usr/include/eina-1/eina

DAEMON_SRCS =			\
	buffer.c		\
	da_data.c		\
	da_debug.c		\
	da_protocol.c		\
	da_protocol_inst.c	\
	da_inst.c		\
	daemon.c		\
	debug.c			\
	elf.c			\
	ioctl_commands.c	\
	main.c			\
	sys_stat.c		\
	threads.c		\
	transfer_thread.c	\
	utils.c			\
	da_protocol_check.c     \
	md5.c			\
	input_events.c		\
	device_vconf.c		\
	device_system_info.c	\
	device_camera.c		\
	smack.c \
	freezing.c

DAEMON_OBJS = $(patsubst %.c,%.o, $(DAEMON_SRCS))

TARGET = da_manager
DASCRIPT = da_command

all: debug
debug release: $(TARGET_LIBS)
release: CPPFLAGS += -DNOLOGI=1
debug release: $(DAEMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $(TARGET)

install: BINDIR = $(DESTDIR)/usr/bin
install: $(TARGET)
	mkdir -p $(BINDIR)
	install $(TARGET) $(DASCRIPT) -t $(BINDIR)

clean:
	rm -f $(TARGET) $(DAEMON_OBJS)

.PHONY: all debug release clean install
