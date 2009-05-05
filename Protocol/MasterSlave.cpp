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
#include <iomanip>
#include <sstream>
#include <cctype>
#include "Utils/Error.h"
#include "Utils/Hash.h"
#include "Protocol.h"
#include "MasterSlave.h"

/************************************
 * General master/slave functions
 ************************************/

MasterSlave::MasterSlave(Protocol::Callback *HigherCB,
				 Protocol &Lower,
				 int Address, int Timeout)
	: HigherCB(HigherCB), Lower(Lower),/* LowerCB(*this),*/ TimeoutCB(*this)
{
	/* Register us in Lowlevel interface */
//	Lower.RegisterCallback(&LowerCB);
	Lower.RegisterCallback(this);

	this->Timeout = Timeout;
	this->Address = Address;
	this->Quiet = false;
}


MasterSlave::~MasterSlave()
{
	/* Deregister our callback */
	Lower.RegisterCallback(NULL);
}


void MasterSlave::RegisterCallback(Protocol::Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}



void MasterSlave::RaiseError(int Errno, const char *Additional) const
{
	std::ostringstream ss;

	if (Quiet)
		return;

	if (Additional)
		ss << "Master/Slave Error: " << Additional;

	std::cerr << Errno /* FIXME: Remove this cerr */
		  << " : "
		  << Error::StrError(Errno)
		  << std::endl;

	if (Additional) {
		std::cerr << Additional << std::endl;
	}

	if (HigherCB) {
		if (Additional)
			HigherCB->Error(Errno, ss.str().c_str());
		else
			HigherCB->Error(Errno, NULL);
	}
}


void MasterSlave::SendMessage(const std::string &Msg, int Address, int Function)
{
	Lower.SendMessage(Msg, Address, Function);
}

/************************************
 * Callbacks for lowlevel interface
 * and for timeout.
 ************************************/


void MasterSlave::ReceivedByte(char Byte)
{
	if (HigherCB)
		HigherCB->ReceivedByte(Byte);
}


void MasterSlave::SentByte(char Byte)
{
	if (HigherCB)
		HigherCB->SentByte(Byte);
}


void MasterSlave::SentMessage(const std::string &Msg, int Address, int Function)
{
	if (HigherCB && !Quiet)
		HigherCB->SentMessage(Msg, Address, Function);
}


void MasterSlave::ReceivedMessage(const std::string &Msg, int Address, int Function)
{
	if (Address != this->Address && Address != 0) {
		std::ostringstream ss;
		ss << "Got message for " << Address
		   << " our is " << this->Address
		   << " ignoring";
		RaiseError(Error::ADDRESS, ss.str().c_str());
		return;
	}

	if (HigherCB)
		HigherCB->ReceivedMessage(Msg, Address, Function);
}


void MasterSlave::Error(int Errno, const char *Description)
{
	std::cerr << "Master/Slave: Got error from lower layer: "
		  << Errno
		  << std::endl;
	if (HigherCB) {
		/* Pass this error to interface with callback */
		HigherCB->Error(Errno, Description);
	}
}


MasterSlave::TimeoutCB::TimeoutCB(MasterSlave &MM) : M(MM)
{
}


void MasterSlave::TimeoutCB::Run()
{
	Notice = 1;
	std::cout << "Not implemented" << std::endl;
}

