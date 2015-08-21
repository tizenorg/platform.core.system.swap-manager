START_SH = scripts/start.sh
STOP_SH = scripts/stop.sh
TARGET = da_manager
DASCRIPT = da_command


install: BINDIR = $(DESTDIR)/usr/bin
install: OPTDIR = $(DESTDIR)/opt/swap/sdk

