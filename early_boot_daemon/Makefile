CC := gcc

CFLAGS := -Werror -g -O0

LDFLAGS := -lpthread

SRC_C := \
	swap_readerd.c \
	buffer.c \
	debug.c \
	swap_config.c

OBJS_C := $(SRC_C:.c=.o)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

OBJS := $(OBJS_C)

TARGET = swap_readerd

all: release

release:  $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

install: BINDIR = $(DESTDIR)/usr/bin
install: $(TARGET)
	mkdir -p $(BINDIR)
	install $(TARGET) -t $(BINDIR)
	sed -i 's|OPT_PART|$(OPT_PART_VAR)|' swap_on_boot.sh
	install swap_on_boot.sh -t $(BINDIR)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all release
