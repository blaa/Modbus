CC=g++
CFLAGS=-O0 -Wall -ggdb

EXE=Comm

OBJECTS=Main.cpp Config.cpp Protocol/Modbus.cpp Protocol/Terminated.cpp \
	Lowlevel/Serial.cpp Interface/Interface.cpp
HEADERS=Config.h Protocol/Protocol.h Protocol/Modbus.h \
	Protocol/Terminated.h \
	Lowlevel/Serial.h Lowlevel/Lowlevel.h Interface/Interface.h

.PHONY: clean

$(EXE): $(OBJECTS:.cpp=.o) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS:.cpp=.o)

%.o: %.cpp %.h
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp 
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJECTS:.cpp=.o) $(EXE)


