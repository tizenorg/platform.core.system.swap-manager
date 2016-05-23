CC := gcc
CPP := g++

COMM_FLAGS := -Wall -g -O0 -Werror

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
	-I. \
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
	-I/usr/include/evas-1 \
	-I/usr/include/dlog

INCLUDE_CPP := \
	-I. \
	-Icpp

# CALL_MNGR
ifeq ($(CALL_MNGR),y)
INCLUDE += -I/usr/include/call-manager/
endif # CALL_MNGR

FLAGS := \
	$(COMM_FLAGS) \
	$(INCLUDE) \
	$(DEBUG_FLAGS)


# set profile info
ifeq ($(PROFILE_TV),y)
FLAGS += -DPROFILE_TV
endif # PROFILE_TV

# set using old json
ifeq ($(OLD_JSON),y)
FLAGS += -DOLD_JSON
endif # OLD_JSON

# CALL_MNGR
ifeq ($(CALL_MNGR),y)
FLAGS += -DCALL_MNGR
endif # CALL_MNGR

ifeq (arm, $(findstring arm, $(shell uname -sm)))
	FLAGS += -DDEVICE_ONLY
endif


# compiler flags
CFLAGS := $(FLAGS)
CPPFLAGS := $(FLAGS) $(INCLUDE_CPP) -std=c++0x

# linker flags
LDFLAGS := \
	-lpthread \
	-lvconf \
	-lsmack \
	-lecore \
	-lcapi-system-info \
	-lwebsockets \
	-lparserelf \
	-ldl \
	-ldlog

# CALL_MNGR
ifeq ($(CALL_MNGR),y)
LDFLAGS += -lcall-manager
endif # CALL_MNGR

# set using old json
ifeq ($(OLD_JSON),y)
LDFLAGS += -ljson
else
LDFLAGS += -ljson-c
endif # OLD_JSON

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
	cpp/inst/Anr.cpp \
\
	cpp/elf/File.cpp \
	cpp/elf/FileElf.cpp

TARGET = da_manager
DASCRIPT = da_command
START_SH = ../scripts/swap_start.sh
STOP_SH = ../scripts/swap_stop.sh
INIT_LOADER_SH = ../scripts/swap_init_loader.sh
INIT_PRELOAD_SH = ../scripts/swap_init_preload.sh
INIT_UIHV_SH = ../scripts/swap_init_uihv.sh
INIT_WSP_SH = ../scripts/swap_init_wsp.sh
LOADER_SCRIPT = ../scripts/gen_loader_header.sh
PRELOAD_SCRIPT = ../scripts/gen_preload_header.sh
UIHV_SCRIPT = ../scripts/gen_uihv_header.sh
SCRIPTS := \
	$(START_SH) \
	$(STOP_SH) \
	$(INIT_LOADER_SH) \
	$(INIT_PRELOAD_SH) \
	$(INIT_UIHV_SH) \
	$(INIT_WSP_SH)

all: debug

GENERATED_DIR = include/generated
GENERATED_WSI_PROF_H = include/generated/wsi_prof.h
GENERATED_NSP_H = include/generated/nsp_data.h
GENERATED_HEADERS := \
	$(GENERATED_WSI_PROF_H) \
	$(GENERATED_NSP_H)

$(GENERATED_WSI_PROF_H): ../scripts/gen_wsi_prof.sh
	sh $< > $@

#Loader
$(INIT_LOADER_SH): $(LOADER_SCRIPT)
	bash $< $@

#Preload
$(INIT_PRELOAD_SH): $(PRELOAD_SCRIPT)
	bash $< $@

#UIHV
$(INIT_UIHV_SH): $(UIHV_SCRIPT)
	bash $< $@

# WSP
$(INIT_WSP_SH): $(WSP_SCRIPT)
ifeq ($(WSP_SUPPORT),y)
	bash $(WSP_SCRIPT) -s > $@
else # WSP_SUPPORT
	echo "#!/bin/bash" > $@
	echo "echo \"Do not init WSP!\" " >> $@
endif # WSP_SUPPORT

ifeq ($(WSP_SUPPORT),y)
  SRC_CPP += cpp/features/feature_wsp.cpp
  GENERATED_WSP_H = include/generated/wsp_data.h
  WSP_SCRIPT = ../scripts/gen_wsp_data.sh
  GENERATED_HEADERS += $(GENERATED_WSP_H)

  $(GENERATED_WSP_H): $(WSP_SCRIPT)
	bash $< -h > $@

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


debug: rmheaders headers $(SCRIPTS) $(OBJS)
	$(CPP) $(OBJS) -o $(TARGET) $(LDFLAGS)


install: BINDIR = $(DESTDIR)/usr/bin
install: $(TARGET) $(SCRIPTS)
	mkdir -p $(BINDIR)
	install $(TARGET) -t $(BINDIR)
	install -m 755 $(SCRIPTS) -t $(BINDIR)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all debug release clean install
