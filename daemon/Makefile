CURDIR = `pwd`
INSTALLDIR = usr/bin

UNAME := $(shell uname -sm)
ifneq (,$(findstring 86,$(UNAME)))
	HOST_ARCH := x86
endif

TARGET_ARCH = $(HOST_ARCH)
ifeq ($(TARGET_ARCH),)
	TARGET_ARCH := arm
endif

ifeq ($(TARGET_ARCH),arm)
	DEVICE = -DDEVICE_ONLY
endif

INC := 	-I/usr/include	\
		-I/usr/include/system \
		-I/usr/include/telephony \
		-I/usr/include/vconf

DAEMON_SRCS :=	\
	main.c		\
	daemon.c	\
	da_protocol.c	\
	threads.c	\
	da_debug.c	\
	da_data.c	\
	utils.c		\
	sys_stat.c

DAEMON_SRCS_HOST :=	\
	main.c		\
	daemon.c	\
	da_protocol.c	\
	threads.c	\
	da_debug.c	\
	da_data.c	\
	utils.c		\
	sys_stat.c


LDLIBS_PATH := -L/usr/lib -L./lib/
LIBS_TIZEN :=# -lcapi-system-runtime-info \
				-lcapi-telephony-network-info \
				-lcapi-telephony-call \
				-lSLP-tapi \
				-lsecurity-server-client \
				-lbundle \
				-lail \
				-lxdgmime \
				-ldlog \
				-lSLP-db-util
LIBS_COMMON :=	-lglib-2.0 \
				-lvconf \
				-lsmack \
				-lcapi-system-info

LIBS_COMMON_HOST :=		-lglib-2.0
HOST_BUILD := -DLOCALTEST=1 -O0 -g
DEBUG_ON := -DDEBUG=1
FLAGS = -pthread -Wall

TARGET = da_manager
DASCRIPT = da_command

all: $(TARGET)

host: $(DAEMON_SRCS_HOST)
	$(CC) $(INC) $(FLAGS) $(DEBUG_ON) $(HOST_BUILD) $(DEVICE) -o $(TARGET) $(DAEMON_SRCS_HOST) $(LDLIBS_PATH) $(LIBS_COMMON_HOST) $(LIBS_TIZEN)

$(TARGET): $(DAEMON_SRCS)
	$(CC) $(INC) $(FLAGS) $(DEBUG_ON) $(DEVICE) -o $@ $(DAEMON_SRCS) $(LDLIBS_PATH) $(LIBS_COMMON) $(LIBS_TIZEN)

localtest:
	$(CC) -DLOCALTEST -DHOST_BUILD $(INC) $(FLAGS) -o $(TARGET) $(DEBUG_ON) $(DEVICE) $(LDLIBS_PATH) $(LIBS_COMMON) $(DAEMON_SRCS)

install:
	[ -d "$(DESTDIR)/$(INSTALLDIR)" ] || mkdir -p $(DESTDIR)/$(INSTALLDIR)
	install $(TARGET) $(DASCRIPT) $(DESTDIR)/$(INSTALLDIR)/

clean:
	rm $(TARGET) 

