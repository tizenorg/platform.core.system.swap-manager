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
	-Iinclude/generated \
	-I/usr/include \
	-I/usr/include/system \
	-I/usr/include/capi-system-info \
	-I/usr/include/capi-system-runtime-info \
	-I/usr/include/vconf \
	-I/usr/include/ecore-1 \
	-I/usr/include/eina-1 \
	-I/usr/include/eina-1/eina \
	-I/usr/include/efl-1 \
	-I/usr/include/json-c \
	-I/usr/include/eo-1 \
	-I/usr/include/evas-1
# CALL_MNGR
ifeq ($(CALL_MNGR),y)
INCLUDE += -I/usr/include/callmgr_client
endif # CALL_MNGR

FLAGS := \
	$(COMM_FLAGS) \
	$(INCLUDE) \
	$(DEBUG_FLAGS)

# CALL_MNGR
ifeq ($(CALL_MNGR),y)
FLAGS += -DCALL_MNGR
endif # CALL_MNGR

ifeq (arm, $(findstring arm, $(shell uname -sm)))
	FLAGS += -DDEVICE_ONLY
endif


# compiler flags
CFLAGS := $(FLAGS)
CPPFLAGS := $(FLAGS) -std=c++0x -I. -Icpp

# linker flags
LDFLAGS := \
	-lpthread \
	-lvconf \
	-lsmack \
	-lecore \
	-lcapi-system-info \
	-lwebsockets \
	-ljson-c \
	-lparserelf

# CALL_MNGR
ifeq ($(CALL_MNGR),y)
LDFLAGS += -lcallmgr_client
endif # CALL_MNGR

SRC_C := \
	buffer.c \
	da_data.c \
	da_debug.c \
	da_protocol.c \
	da_protocol_inst.c \
	da_inst.c \
	daemon.c \
	debug.c \
	elf_aux.c \
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
	wsi.c \
	ui_viewer.c

SRC_CPP := \
	cpp/features/feature.cpp \
	cpp/features/feature_manager.cpp \
	cpp/features/feature_manager_c.cpp \
	cpp/features/feature_nsp.cpp \
\
	cpp/inst/AppInst.cpp \
	cpp/inst/AppInstCont.cpp \
\
	cpp/elf/File.cpp \
	cpp/elf/FileElf.cpp

TARGET = da_manager
DASCRIPT = da_command
START_SH = ../scripts/start.sh
STOP_SH = ../scripts/stop.sh
INIT_PRELOAD_SH = ../scripts/init_preload.sh
PRELOAD_SCRIPT = ../scripts/gen_preload_header.sh

all: debug

GENERATED_DIR = include/generated
GENERATED_WSI_PROF_H = include/generated/wsi_prof.h
GENERATED_NSP_H = include/generated/nsp_data.h
GENERATED_HEADERS := \
	$(GENERATED_WSI_PROF_H) \
	$(GENERATED_NSP_H)

$(GENERATED_WSI_PROF_H): ../scripts/gen_wsi_prof.sh
	sh $< > $@


# WSP
ifeq ($(WSP_SUPPORT),y)
  SRC_CPP += cpp/features/feature_wsp.cpp
  GENERATED_WSP_H = include/generated/wsp_data.h
  GENERATED_HEADERS += $(GENERATED_WSP_H)

  $(GENERATED_WSP_H): ../scripts/gen_wsp_data.sh
	sh $< > $@
endif # WSP_SUPPORT


$(GENERATED_NSP_H): ../scripts/gen_nsp_data.sh
	sh $< > $@

$(GENERATED_DIR):
	mkdir -p $(GENERATED_DIR)

headers: $(GENERATED_DIR) $(GENERATED_HEADERS)
rmheaders:
	rm -f $(GENERATED_HEADERS)


release: CFLAGS += -DNOLOGI=1
release: CPPFLAGS += -DNOLOGI=1
release: debug

OBJS_C := $(SRC_C:.c=.o)
OBJS_CPP := $(SRC_CPP:.cpp=.o)
OBJS := $(OBJS_C) $(OBJS_CPP)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(CPP) $(CPPFLAGS) -c -o $@ $<


debug: rmheaders headers $(OBJS)
	$(CPP) $(OBJS) -o $(TARGET) $(LDFLAGS)


install: BINDIR = $(DESTDIR)/usr/bin
install: OPTDIR = $(DESTDIR)/opt/swap/sdk
install: $(TARGET)
	mkdir -p $(BINDIR)
	mkdir -p $(OPTDIR)
	install $(TARGET) -t $(BINDIR)
	bash $(PRELOAD_SCRIPT) $(INIT_PRELOAD_SH)
	install -m 755 $(INIT_PRELOAD_SH) $(START_SH) $(STOP_SH) -t $(OPTDIR)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all debug release clean install
