##
# Configuration
##
CC=g++
CFLAGS=-O0 -Wall -ggdb -I.

EXE=Comm

SOURCES=Main.cpp Config.cpp Utils/Error.cpp Protocol/Modbus.cpp Protocol/Terminated.cpp \
	Lowlevel/Lowlevel.cpp Interface/Interface.cpp Utils/Timeout.cpp \
	Lowlevel/SerialLinux.cpp Lowlevel/SerialDOS.cpp 

HEADERS=Config.h Protocol/Protocol.h Protocol/Modbus.h \
	Protocol/Terminated.h \
	Lowlevel/Lowlevel.h Interface/Interface.h Utils/Error.h \
	Utils/CRC.h Utils/TImeout.h Lowlevel/SerialLinux.h Lowlevel/SerialDOS.h 

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
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

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

