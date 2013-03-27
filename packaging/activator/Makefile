
HEADER = WindowActivator.h

SRCS = main.cpp WindowActivator.cpp

TARGET = WindowActivator

FLAGS = -Wall

#LDFLAGS = -lstdc++ -lX11
LDFLAGS = -lstdc++ -framework IOKit -framework ApplicationServices

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADER)
	$(CC) -o $@ $(FLAGS) $(SRCS) $(LDFLAGS)

clean:
	rm $(TARGET)

