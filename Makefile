##
# Configuration
##
CC=g++
CFLAGS=-O0 -Wall -ggdb -I.
LDFLAGS=
BOOST=1

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
# Helper targets
##

%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJECTS) $(EXE)

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

