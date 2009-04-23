#include <iostream>
#include <cctype>
#include "Utils/Error.h"
#include "Utils/Hash.h"
#include "Modbus.h"

/************************************
 * Main modbus ascii class implementation 
 ************************************/
Modbus::Modbus(Callback *CB, Lowlevel &LL) : C(CB), L(LL), LCB(*this)
{
	/* Register us in Lowlevel interface */
	LL.RegisterCallback(&LCB);
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

	/* Store here bytes we can't convert yet */
	static char HalfByte;
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
		LRC::Hash_t ReceivedLRC = Buffer[Buffer.length()-1];
		Buffer.erase(Buffer.length()-1); /* TODO: optimize this, it runs in O(n) */
		
		for (int i = 0; i<Buffer.length(); i++) {
			Hash.Update(Buffer[i]);
		}

		/* Real end of message - check Hash */
		if (Hash.State == ReceivedLRC) {
			std::cerr << "Final Hash = " 
				  << std::hex << (unsigned int)Hash.State
				  << std::dec << std::endl;
			Reset();
			RaiseError(Error::HASH);
			return;
		}
		
		/* Hash OK. */ 
		/* FIXME: Check minimal size (address, function) */
		if (C) {
			/* TODO: remove CRC */
			C->ReceivedMessage(Address, Function, Buffer);
			Reset();
			return;
		}
		/* TODO: No callback installed, yet we've got message
		 * Do something about it */
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
