CC :=gcc
CPPFLAGS := 	-I/usr/include		\
		-I/usr/include/system 	\
		-I/usr/include/telephony\
		-I/usr/include/vconf	\
		-DDEBUG=1
ifeq (arm, $(findstring arm, $(shell uname -sm)))
	CPPFLAGS += -DDEVICE_ONLY
endif

HOST_CPPFLAGS := -DLOCALTEST=1 -O0 -g
COMMON_LIBS := -lglib-2.0 -lpthread
TARGET_LIBS := $(COMMON_LIBS) -lvconf -lsmack -lcapi-system-info
CFLAGS := -Wall -pthread -g -O0

DAEMON_SRCS =			\
	buffer.c		\
	da_data.c		\
	da_debug.c		\
	da_protocol.c		\
	daemon.c		\
	debug.c			\
	elf.c			\
	ioctl_commands.c	\
	main.c			\
	process_info.c		\
	sys_stat.c		\
	threads.c		\
	transfer_thread.c	\
	utils.c			\

DAEMON_OBJS = $(patsubst %.c,%.o, $(DAEMON_SRCS))

TARGET = da_manager
DASCRIPT = da_command

all: target-nolog
host: $(COMMON_LIBS)
target target-nolog: $(TARGET_LIBS)
target-nolog: CPPFLAGS+=-DNOLOGI=1
host: CPPFLAGS+=$(HOST_CPPFLAGS)
target host target-nolog: $(DAEMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $(TARGET)

install: BINDIR=$(DESTDIR)/usr/bin
install: target
	mkdir -p $(BINDIR)
	install $(TARGET) $(DASCRIPT) -t $(BINDIR)

clean:
	rm -f $(TARGET) $(DAEMON_OBJS)

.PHONY: all host target clean install target-nolog
