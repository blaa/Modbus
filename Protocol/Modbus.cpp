#include <iostream>
#include <cctype>
#include "Utils/Error.h"
#include "Utils/Hash.h"
#include "Modbus.h"

/************************************
 * Main modbus ascii class implementation 
 ************************************/
Modbus::Modbus(Callback *CB, Lowlevel &LL, int Timeout) 
  : C(CB), L(LL), LCB(*this), TCB(*this)
{
	/* Register us in Lowlevel interface */
	LL.RegisterCallback(&LCB);

	this->Timeout = Timeout;
}

Modbus::~Modbus()
{
	/* Deregister our callback */
	L.RegisterCallback(NULL);
}


void Modbus::RegisterCallback(Callback *C)
{
	this->C = C;
}


void Modbus::SendMessage(const std::string &Msg, int Address)
{
	/* Form new buffer with modbus thingies and pass to lowlevel */
}

void Modbus::Reset()
{
	HalfByte = 0;
	Received = 0;
	Buffer.clear();
	Hash.Init();
}

unsigned char Modbus::AAHexConvert(unsigned char A, unsigned char B)
{
	unsigned char Result;
	if (A >= '0' && A <= '9')
		A -= '0';
	else {
		if (A >= 'A' && A <= 'F')
			A = A - 'A' + 10;
	}

	if (B >= '0' && B <= '9')
		B -= '0';
	else {
		if (B >= 'A' && B <= 'F')
			B = B - 'A' + 10;
	}
	Result = A * 16 + B;
	return Result;
}


/** Modbus ASCII frame grabber */
void Modbus::ByteReceived(char Byte)
{
	/* Modbus ASCII Frame:
	 * :<ADDRESS><FUNCTION><DATA><LRC><CR><LF>
	 * Except for initial : and CRLF whole message must be converted
	 * from two characters into one.
	 */


	/* We've got some byte - reset timeout 
	 * so we won't get Reset() during this function */
	Timeout::Register(&this->TCB, this->Timeout, 0);

	if (0 == Received) {
		/* Buffer is empty; byte must equal ':' */
		if (Byte != ':') {
			/* Frame error */
			RaiseError(Error::FRAME, "Frame does not start with a colon");
			return;
		}
		this->Hash.Init();
//		this->Hash.Update(Byte); /* FIXME: For sure include ':' in LRC? */
		Received++;
		return;
	}

	/* Handle end of message */
	if (Byte == '\n') {
		if (HalfByte != '\r') {
			/* Frame error \n without \r */
			Reset();
			RaiseError(Error::FRAME, "LF without preceding CR");
			return;
		}
		
		/* Calculate LRC for message body */
		const LRC::Hash_t ReceivedLRC = Buffer[Buffer.length()-1];
		Buffer.erase(Buffer.length()-1); /* TODO: optimize this, it runs in O(n) */
		
		for (unsigned int i = 0; i<Buffer.length(); i++) {
			Hash.Update(Buffer[i]);
		}

		/* Real end of message - check Hash */
		if (Hash.Get() != ReceivedLRC) {
			std::cerr << "Final Hash = " 
				  << std::hex << (unsigned int)Hash.State
				  << std::dec << std::endl;
			Reset();
			RaiseError(Error::HASH);
			return;
		}
		
		/* Hash OK. */ 
		/* FIXME: Check minimal size (address, function)
		 * ^^ - should not be needed - LRC check will fail
		 */
		if (C) {
			C->ReceivedMessage(Address, Function, Buffer);
			Reset();
			return;
		}

		/* TODO: No callback installed, yet we've got message
		 * Do something about it */
	} else if (HalfByte == '\r') {
		/* We don't get \n, yet we received \r previously - error */
		Reset();
		RaiseError(Error::FRAME, "CR without succeding LF");
		return;
	}

	if (Byte == '\r') {
		/* Previous value was a LRC! */
		Received++;
		HalfByte = '\r';
		return;
	}

	/* Byte = [0-9A-F\n\r] */
	if (!isdigit(Byte) && (Byte<'A' || Byte>'F')) {
		/* Byte out of range */
		Reset();
		RaiseError(Error::FRAME, "Incorrect byte in frame body");
		return;
	}

	/* Got probably correct data.
	 * If Received in this point odd this is the 
	 * first characted of byte we are supposed to gather.
	 * Then we will only remember it. 
	 * Else - we have to match it with previous character,
	 * convert and correctly store.
	 */
	if (Received % 2 != 0) {
		HalfByte = Byte;
		Received++;
		return;
	} 

	/* Convert */
	HalfByte = AAHexConvert(HalfByte, Byte);
	Received++;

	std::cerr << "Converted = " 
		  << std::hex << (unsigned int) (unsigned char) HalfByte << std::dec
		  << std::endl;

	switch (Received) {
	case 3:
		/* This is address byte */
		Hash.Update(HalfByte);
		Address = HalfByte;
		return;
	case 5:
		/* This is function byte */
		Hash.Update(HalfByte);
		Function = HalfByte;
		return;
	default:
		/* Message byte - these Hash will be calculated later */
		Buffer.push_back(HalfByte);
		return;
	}
}


void Modbus::RaiseError(int Errno, const char *Additional) const
{
	/* Turn off timeout - no frame incoming */
	Timeout::Register(NULL, this->Timeout, 0);


	/* TODO: Turn this debug off finally */
	std::cerr << "MODBUS Error: "
		  << Errno 
		  << " : "
		  << Error::StrError(Errno)
		  << std::endl;
	if (Additional) {
		std::cerr << Additional << std::endl;
	}

	if (C) {
		C->Error(Errno);
		return;
	}
}


/************************************
 * Callbacks for lowlevel interface
 * and for timeout.
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


Modbus::TimeoutCallback::TimeoutCallback(Modbus &MM) : M(MM)
{
}

void Modbus::TimeoutCallback::Run()
{
	Notice = 1;
	/* Timeout! Reset receiver */
	M.Reset();
	M.RaiseError(Error::TIMEOUT);
}
