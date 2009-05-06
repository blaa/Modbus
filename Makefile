##
# Configuration
##
UBUNTU=0


# Use QT Interface
QT=1
CC=g++
CFLAGS=-O0 -Wall -ggdb -I.
LDFLAGS=
EXE=Comm

# QT Config
UIC=uic
MOC=moc
QTINC=/usr/include/qt4
QTLIB=/usr/lib/qt4

# Overwrite here anything which must be set differently for Ubuntu
ifeq ($(UBUNTU), 1)
UIC=uic-qt4
MOC=moc-qt4
#QTINC=
#QTLIB=
endif

SOURCES=Main.cpp Config.cpp Utils/Error.cpp Utils/Timeout.cpp \
	Lowlevel/Lowlevel.cpp Lowlevel/SerialLinux.cpp \
	Lowlevel/SerialDOS.cpp \
	Lowlevel/NetworkTCP.cpp Lowlevel/Network.cpp Lowlevel/NetworkUDP.cpp \
	Protocol/Modbus.cpp Protocol/Terminated.cpp Protocol/MasterSlave.cpp \
	Interface/Interface.cpp

##
# Generated data
##
ifeq ($(QT), 1)
CFLAGS+=-DQT_INTERFACE -I$(QTINC)
SOURCES+=Interface/QT/ModbusFrame.cpp Interface/QT/moc_ModbusFrame.cpp \
	Utils/moc_Timeout.cpp
LDFLAGS+=-L$(QTLIB) -lQtGui
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
	$(UIC) -o $@ $<

Interface/QT/moc_ModbusFrame.cpp: Interface/QT/ModbusFrame.h Interface/QT/ui_ModbusFrame.h
	$(MOC) -DQT_INTERFACE=1 $< > $@

Utils/moc_Timeout.cpp: Utils/Timeout.h
	$(MOC) -DQT_INTERFACE=1 $< > $@

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

