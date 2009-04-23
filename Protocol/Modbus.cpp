#include <iostream>
#include "Utils/Error.h"
#include "Utils/Hash.h"
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
	/* Form new buffer with modbus thingies and pass to lowlevel */
}


void Modbus::ByteReceived(char Byte)
{
	if (0 == Received) {
		/* Buffer is empty; byte must equal ':' */
		if (Byte != ':') {
			/* Frame error */
			RaiseError(Error::FRAME);
			return;
		}
		Received++;
	}
}


void Modbus::RaiseError(int Errno)
{
	/* TODO: Turn this debug off finally */
	std::cerr << "MODBUS Error: "
		  << Errno 
		  << " : "
		  << Error::StrError(Errno)
		  << std::endl;

	if (C) {
		C->Error(Error::FRAME);
		return;
	}
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
