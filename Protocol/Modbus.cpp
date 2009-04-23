#include <iostream>
#include "Modbus.h"

/************************************
 * Main modbus ascii class implementation 
 ************************************/


Modbus::Modbus(Callback *CB, Lowlevel &LL) : C(CB), L(LL)
{
}

void Modbus::RegisterCallback(Callback *C)
{
	this->C = C;
}


void Modbus::SendMessage(const std::string &Msg, int Address)
{
	/* Form new buffer with modbus thingies and pass to serial */
}


void Modbus::ByteReceived(char Byte)
{
	int 
}


/************************************
 * Callback for lowlevel interface
 ************************************/
Modbus::LowlevelCallback::LowlevelCallback(Modbus &MM) : M(MM)
{
}

void Modbus::LowlevelCallback::ByteReceived(char Byte)
{
	M.ByteReceived(Byte);
}

void Modbus::LowlevelCallback::Error(int Errno)
{
	std::cerr << "Got error from low layer: "
		  << Errno 
		  << std::endl;
	if (M.C) {
		/* Pass this error to interface with callback */
		M.C->Error(Errno);
	}
}
