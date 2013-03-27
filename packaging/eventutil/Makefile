CURDIR = `pwd`
INSTALLDIR = usr/bin

SRCS :=	eventutil.c

TARGET = da_event

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) -Wall -o $@ $(SRCS)

install:
	[ -d "$(DESTDIR)/$(INSTALLDIR)" ] || mkdir -p $(DESTDIR)/$(INSTALLDIR)
	install $(TARGET) $(DESTDIR)/$(INSTALLDIR)/

clean:
	rm $(TARGET) 

