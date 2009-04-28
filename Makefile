##
# Configuration
##

# Use QT Interface
QT=1
CC=g++
CFLAGS=-O0 -Wall -ggdb -I.
LDFLAGS=

EXE=Comm

SOURCES=Main.cpp Config.cpp Utils/Error.cpp Utils/Timeout.cpp \
	Lowlevel/Lowlevel.cpp Lowlevel/SerialLinux.cpp \
	Lowlevel/SerialDOS.cpp Lowlevel/Network.cpp \
	Protocol/Modbus.cpp Protocol/Terminated.cpp Protocol/MasterSlave.cpp \
	Interface/Interface.cpp

HEADERS=Config.h Utils/Timeout.h Utils/CRC.h Utils/Error.h \
	Lowlevel/Lowlevel.h Lowlevel/SerialLinux.h \
	Lowlevel/SerialDOS.h Lowlevel/Network.h \
	Protocol/Protocol.h Protocol/Modbus.h Protocol/Terminated.h Protocol/MasterSlave.h \
	Interface/Interface.h
#
##
# Generated data
##
ifeq ($(QT), 1)
CFLAGS+=-DQT_INTERFACE -I/usr/include/qt4
SOURCES+=Interface/QT/ModbusFrame.cpp Interface/QT/moc_ModbusFrame.cpp
HEADERS+=Interface/QT/ui_ModbusFrame.h
LDFLAGS+=-L/usr/lib/qt4 -lQtGui
endif

DEPS=$(SOURCES:.cpp=.d)
OBJECTS=$(SOURCES:.cpp=.o)


.PHONY: all clean docs distclean

# Default target
all: $(EXE)

# Load dependecy description
-include $(DEPS)

##
# Global targets
##
$(EXE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJECTS)

docs:
	doxygen

##
# QT Files
##

ifeq "$(QT)" "1"
Interface/QT/ui_ModbusFrame.h: Interface/QT/ModbusFrame.ui
	uic -o $@ $<

Interface/QT/moc_ModbusFrame.cpp: Interface/QT/ModbusFrame.h Interface/QT/ui_ModbusFrame.h
	moc $< > $@

Inteface/QT/ModbusFrame.cpp: Interface/QT/ModbusFrame.h
	$(CC) -c $(CFLAGS) -o $@ $<
endif

##
# Helper targets
##

%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJECTS) $(DEPS) $(EXE) 
	rm -f Interface/QT/moc_ModbusFrame.cpp Interface/QT/ui_ModbusFrame.h
	find . -type f -name "*.d.[0-9]*" -exec rm -f {} \;

distclean: clean
	rm -f $(DEPS) 
	rm -rf Docs/html Docs/latex

# Building dependencies
%.d: %.cpp
	@echo '(DEP) $< -> $@'
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

