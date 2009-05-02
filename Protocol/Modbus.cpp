/**********************************************************************
 * Comm -- Connection framework
 * (C) 2009 by Tomasz bla Fortuna <bla@thera.be>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * See Docs/LICENSE
 *********************/

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
ModbusGeneric<HashType, ASCII>::ModbusGeneric(Callback *HigherCB, 
					      Lowlevel &Lower, int Timeout)
  : HigherCB(HigherCB), Lower(Lower), LowerCB(*this), TimeoutCB(*this)
{
	/* Register us in Lowlevel interface */
	Lower.RegisterCallback(&LowerCB);
	this->Timeout = Timeout;
	Reset();
}

template<typename HashType, bool ASCII>
ModbusGeneric<HashType, ASCII>::~ModbusGeneric()
{
	/* Deregister our callback */
	Lower.RegisterCallback(NULL);
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
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
	Frame << std::setw(2) << (unsigned int)(unsigned char)Hash.Get();
	Frame << "\r\n";

	std::cerr << "DEBUG, SendMessage ASCII: "
		  << Frame.str() << std::endl;

	/* TODO: We can send this data completely with
	 * interrupts but this would make Serial a bit 
	 * harder to write */
	Lower.SendString(Frame.str());
	if (HigherCB) {
		HigherCB->SentMessage(Msg, Address, Function);
	}
}

/* Partial specialization for Modbus RTU */
template<>
void ModbusGeneric<CRC16, false>::SendMessage(const std::string &Msg, int Address, int Function)
{
	CRC16 Hash;
	std::ostringstream Frame;

	/* Wait until previous send is over */
	if (RTUGap)
		Timeout::Wait();
	RTUGap = false;

	Hash.Init();
	Frame << (unsigned char)Address << (unsigned char)Function;
	Hash.Update(Address);
	Hash.Update(Function);

	for (std::string::const_iterator i = Msg.begin();
	     i != Msg.end();
	     i++) {
		Hash.Update(*i);
		Frame << *i;
	}

	CRC16::Hash_t CRC = Hash.Get();
	Frame << (unsigned char)(CRC & 0x00FF);
	Frame << (unsigned char)(CRC>>8);

	std::cerr << "DEBUG, SendMessage RTU: "
		  << Frame.str() << std::endl;

	/* TODO: We can send this data completely with
	 * interrupts but this would make Serial a bit 
	 * harder to write */
	Lower.SendString(Frame.str());

	/* TODO: This should be done with a Register() 
	 * so we will return immediately, but won't 
	 * allow sending another frame before timeout.
	 */

	/* Enabling this timeout will cause testcase to fail
	 * with a looped output -> input */
	RTUGap = true;
	Timeout::Register(&this->TimeoutCB, Timeout * 3.5);

	if (HigherCB) {
		HigherCB->SentMessage(Msg, Address, Function);
	}
}


template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::Reset()
{
	Timeout::Register(NULL, 0); /* Disable our previous timeout */
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
template<>
void ModbusGeneric<LRC, true>::ReceivedByte(char Byte)
{
	/* Modbus ASCII Frame:
	 * :<ADDRESS><FUNCTION><DATA><LRC><CR><LF>
	 * Except for initial : and CRLF whole message must be converted
	 * from two characters into one.
	 */


	/* We've got some byte - reset timeout 
	 * so we won't get Reset() during this function */
	Timeout::Register(&this->TimeoutCB, this->Timeout);

	if (0 == Received) {
		/* Buffer is empty; byte must equal ':' */
		if (Byte != ':') {
			/* Frame error */
			RaiseError(Error::FRAME, "Frame does not start with a colon");
			return;
		}
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
			Reset();
			std::cerr << "LRC was supposed to equal "
				  << std::hex
				  << (unsigned int) (unsigned char) ReceivedLRC
				  << "it equals "
				  << (unsigned int) (unsigned char) Hash.Get()
				  << std::dec
				  << std::endl;
			RaiseError(Error::HASH);
			return;
		}
		
		/* Hash OK. */ 
		/* FIXME: Check minimal size (address, function)
		 * ^^ - should not be needed - LRC check will fail
		 */
		if (HigherCB) {
			HigherCB->ReceivedMessage(Buffer, Address, Function);
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

/** Modbus RTU frame grabber */
template<>
void ModbusGeneric<CRC16, false>::ReceivedByte(char Byte)
{
	/* Modbus ASCII Frame:
	 * :<ADDRESS><FUNCTION><DATA><LRC><CR><LF>
	 * Except for initial : and CRLF whole message must be converted
	 * from two characters into one.
	 */

	/* Something happened - reset timeout. When it reaches us 
	 * we can either be Reset() if CRC is incorrect or we can mark
	 * the correct frame */
	Timeout::Register(&this->TimeoutCB, this->Timeout * 1.5);
	RTUGap = false;
	/* FIXME: This register will overwrite possible send-lock wait! */

	Received++;

	this->Hash.Update(Byte);
	
	switch (Received) {
	case 1:
		/* This is address byte */
		Address = Byte;
		return;
	case 2:
		/* This is function byte */
		Function = Byte;
		return;
	default:
		/* Message byte (+ 2 bytes of CRC which has to be stripped!) */
		Buffer.push_back(Byte);
		return;
	}
}


template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::RaiseError(int Errno, const char *Additional) const
{
	std::ostringstream ss;
	/* Turn off timeout - no frame incoming */
	Timeout::Register(NULL, this->Timeout);

	if (!HigherCB)
		return;

	/* TODO: Turn this debug off finally */
	if (ASCII && Additional) 
		ss << "MODBUS ASCII Error: " << Additional;
	else if (Additional)
		ss << "MODBUS RTU Error: " << Additional;

	if (Additional)
		HigherCB->Error(Errno, ss.str().c_str());
	else
		HigherCB->Error(Errno, NULL);
	return;
}


/************************************
 * Callbacks for lowlevel interface
 * and for timeout.
 ************************************/
template<typename HashType, bool ASCII>
ModbusGeneric<HashType, ASCII>::LowerCB::LowerCB(ModbusGeneric<HashType, ASCII> &MM) : M(MM)
{
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::LowerCB::ReceivedByte(char Byte)
{
	/* Inform higher layer about this single byte */
	if (M.HigherCB) {
		M.HigherCB->ReceivedByte(Byte);
	}

	M.ReceivedByte(Byte);
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::LowerCB::SentByte(char Byte)
{
	/* Inform higher layer about this single byte */
	if (M.HigherCB) {
		M.HigherCB->SentByte(Byte);
	}
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::LowerCB::Error(int Errno)
{
	std::cerr << "Got error from low layer: "
		  << Errno 
		  << std::endl;
	if (M.HigherCB) {
		/* Pass this error to interface with callback */
		M.HigherCB->Error(Errno, "Error from lowlevel");
	}
}

template<typename HashType, bool ASCII>
ModbusGeneric<HashType, ASCII>::TimeoutCB::TimeoutCB(ModbusGeneric<HashType, ASCII> &MM) : M(MM)
{
}

template<typename HashType, bool ASCII>
void ModbusGeneric<HashType, ASCII>::TimeoutCB::Run()
{
	/* Timeout might be set during receive or during sent */

	/* Timeout! Reset receiver */
	if (ASCII) {
		M.Reset();
		M.RaiseError(Error::TIMEOUT);
	} else {
		if (M.RTUGap) {
			M.RTUGap = false;
			/* Ready to send another frame */
			return;
		}
		/*
		 * In RTU we either reset the receiver
		 * if current frame is broken (CRC not correct)
		 * Or we inform higher layer about correct frame 
		 */
		if (M.Hash.IsCorrect()) {
			M.Buffer.erase(M.Buffer.length()-2, M.Buffer.length()-1);
			if (M.HigherCB) {
				M.HigherCB->ReceivedMessage(M.Buffer, M.Address, M.Function);
			}
		} else {
			if (M.Received < 4) 
				M.RaiseError(Error::TIMEOUT, "RTU Timeout - too little read to check CRC");
			else 
				M.RaiseError(Error::HASH, "RTU CRC check failed after reading frame");
		}
		M.Reset();
		return;
	}
}


/**@{ Explicit template specialization */
template class ModbusGeneric<LRC, true>;
template class ModbusGeneric<CRC16, false>;
/*@}*/
