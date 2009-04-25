#include <iostream>
#include <iomanip> /* std::setw std::setfill*/
#include <sstream>
#include <cctype>
#include "Utils/Error.h"
#include "Utils/Hash.h"
#include "Modbus.h"

/************************************
 * Main modbus ascii class implementation 
 ************************************/
template<typename HashType, bool ASCII>
ModbusGeneric<HashType, ASCII>::ModbusGeneric(Callback *CB, Lowlevel &LL, int Timeout) 
  : C(CB), L(LL), LCB(*this), TCB(*this)
{
	/* Register us in Lowlevel interface */
	LL.RegisterCallback(&LCB);

	this->Timeout = Timeout;
}

template<typename HashType, bool ASCII>
ModbusGeneric<HashType, ASCII>::~ModbusGeneric()
{
	/* Deregister our callback */
	L.RegisterCallback(NULL);
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::RegisterCallback(Callback *C)
{
	this->C = C;
}

/* Partial specialization for Modbus ASCII */
template<>
void ModbusGeneric<LRC, true>::SendMessage(const std::string &Msg, int Address, int Function)
{
	LRC Hash;
	std::ostringstream Frame;
	Hash.Init();
	Frame << std::hex << std::setfill('0') << std::uppercase;
	Frame << ":" 
	      << std::setw(2) << Address 
	      << std::setw(2) << Function;
	Hash.Update(Address);
	Hash.Update(Function);
	for (std::string::const_iterator i = Msg.begin();
	     i != Msg.end();
	     i++) {
		Hash.Update(*i);
		Frame << std::setw(2) << (unsigned int)(unsigned char) *i;
	}
	Frame << (unsigned int)(unsigned char)Hash.Get();
	Frame << "\r\n";

	std::cerr << "DEBUG, SendMessage ASCII: "
		  << Frame.str() << std::endl;

	/* TODO: We can send this data completely with
	 * interrupts but this would make Serial a bit 
	 * harder to write */
	L.SendString(Frame.str());
}

/* Partial specialization for Modbus RTU */
template<>
void ModbusGeneric<CRC16, false>::SendMessage(const std::string &Msg, int Address, int Function)
{
	CRC16 Hash;
	std::ostringstream Frame;
	Hash.Init();
	Frame << Address << Function;
	Hash.Update(Address);
	Hash.Update(Function);
	for (std::string::const_iterator i = Msg.begin();
	     i != Msg.end();
	     i++) {
		Hash.Update(*i);
		Frame << *i;
	}

	CRC16::Hash_t CRC = Hash.Get();
	Frame << (unsigned char)(CRC>>8);
	Frame << (unsigned char)(CRC & 0x00FF);

	std::cerr << "DEBUG, SendMessage RTU: "
		  << Frame.str() << std::endl;

	/* TODO: We can send this data completely with
	 * interrupts but this would make Serial a bit 
	 * harder to write */
	L.SendString(Frame.str());

	/* TODO: Set timeout which will make us wait 
	 * for some time until previous frame finishes */
}


template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::Reset()
{
	HalfByte = 0;
	Received = 0;
	Buffer.clear();
	Hash.Init();
}

template<typename HashType, bool ASCII>
unsigned char ModbusGeneric<HashType, ASCII>::HexConvert(unsigned char A, unsigned char B)
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
template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::ByteReceived(char Byte)
{
	/* Modbus ASCII Frame:
	 * :<ADDRESS><FUNCTION><DATA><LRC><CR><LF>
	 * Except for initial : and CRLF whole message must be converted
	 * from two characters into one.
	 */


	/* We've got some byte - reset timeout 
	 * so we won't get Reset() during this function */
	Timeout::Register(&this->TCB, this->Timeout);

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
	HalfByte = HexConvert(HalfByte, Byte);
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

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::RaiseError(int Errno, const char *Additional) const
{
	/* Turn off timeout - no frame incoming */
	Timeout::Register(NULL, this->Timeout);


	/* TODO: Turn this debug off finally */
	if (ASCII) 
		std::cerr << "MODBUS ASCII Error: ";
	else 
		std::cerr << "MODBUS RTU Error: ";

	std::cerr << Errno 
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
template<typename HashType, bool ASCII>
ModbusGeneric<HashType, ASCII>::LowlevelCallback::LowlevelCallback(ModbusGeneric<HashType, ASCII> &MM) : M(MM)
{
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::LowlevelCallback::ByteReceived(char Byte)
{
	M.ByteReceived(Byte);
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::LowlevelCallback::Error(int Errno)
{
	std::cerr << "Got error from low layer: "
		  << Errno 
		  << std::endl;
	if (M.C) {
		/* Pass this error to interface with callback */
		M.C->Error(Errno);
	}
}

template<typename HashType, bool ASCII>
ModbusGeneric<HashType, ASCII>::TimeoutCallback::TimeoutCallback(ModbusGeneric<HashType, ASCII> &MM) : M(MM)
{
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::TimeoutCallback::Run()
{
	Notice = 1;
	/* Timeout! Reset receiver */
	M.Reset();
	M.RaiseError(Error::TIMEOUT);
}



/**@{ Explicit template specialization */
template class ModbusGeneric<LRC, true>;
template class ModbusGeneric<CRC16, false>;
/*@}*/
