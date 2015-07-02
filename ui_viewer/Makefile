CC := gcc

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
	-I/usr/include

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
                -Wno-psabi              \
                -Werror

FLAGS := \
	$(COMM_FLAGS) \
	$(INCLUDE) \
	$(DEBUG_FLAGS) \
	$(WARN_CFLAGS)

ifeq (arm, $(findstring arm, $(shell uname -sm)))
	FLAGS += -DDEVICE_ONLY
endif


# compiler flags
CFLAGS := $(FLAGS)

# linker flags
LDFLAGS := \
	-shared \
	-lpthread

SRC_C := \
	ui_viewer_lib.c \
	ui_viewer_utils.c

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

