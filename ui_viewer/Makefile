CC := gcc

COMM_FLAGS := -Wall -g -O0 -fPIC

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
	-I/usr/include/elementary-1 \
	-I/usr/include/efl-1 \
	-I/usr/include/eina-1 \
	-I/usr/include/eina-1/eina \
	-I/usr/include/eet-1 \
	-I/usr/include/evas-1 \
	-I/usr/include/eo-1 \
	-I/usr/include/ecore-1 \
	-I/usr/include/ecore-evas-1 \
	-I/usr/include/ecore-file-1 \
	-I/usr/include/ecore-input-1 \
	-I/usr/include/ecore-imf-1 \
	-I/usr/include/ecore-con-1 \
	-I/usr/include/edje-1 \
	-I/usr/include/eldbus-1 \
	-I/usr/include/efreet-1 \
	-I/usr/include/ethumb-client-1 \
	-I/usr/include/ethumb-1 \
	-I/usr/include/e_dbus-1 \
	-I/usr/include/dbus-1.0 \
	-I/usr/lib/dbus-1.0/include

WARN_CFLAGS = -g                        \
                -Wall                   \
                -funwind-tables         \
                -fomit-frame-pointer    \
                -Xlinker                \
                --no-undefined          \
                -Wextra                 \
                -O2                     \
                -Wwrite-strings         \
                -Wlogical-op            \
                -Wpacked                \
                -Winline                \
                -Wno-psabi
#                -Werror

FLAGS := \
	$(COMM_FLAGS) \
	$(INCLUDE) \
	$(DEBUG_FLAGS) \
	$(WARN_CFLAGS)

ifeq (arm, $(findstring arm, $(shell uname -sm)))
	FLAGS += -DDEVICE_ONLY
endif


# compiler flags
CFLAGS := $(FLAGS) -D_GNU_SOURCE

# linker flags
LDFLAGS := \
	-shared \
	-lpthread \
	-lecore \
	-levas \
	-lelementary \
	-ldl


SRC_C := \
	ui_viewer_lib.c \
	ui_viewer_utils.c \
	ui_viewer_data.c \
	ui_viewer_screenshot.c

TARGET = da_ui_viewer.so

all: debug

release: CFLAGS += -DNOLOGI=1
release: debug

OBJS := $(SRC_C:.c=.o)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

debug:  $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)


install: BINDIR = $(DESTDIR)/usr/lib
install: $(TARGET)
	mkdir -p $(BINDIR)
	install $(TARGET) -t $(BINDIR)
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all debug release clean install

