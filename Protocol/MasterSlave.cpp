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
template<bool Master>
MasterSlave<Master>::MasterSlave(Protocol::Callback *HigherCB, Protocol &Lower, int Timeout) 
  : HigherCB(HigherCB), Lower(Lower), LowerCB(*this), TimeoutCB(*this)
{
	/* Register us in Lowlevel interface */
	Lower.RegisterCallback(&LowerCB);
	this->Timeout = Timeout;
}

template<bool Master>
MasterSlave<Master>::~MasterSlave()
{
	/* Deregister our callback */
	Lower.RegisterCallback(NULL);
}

template<bool Master>
void MasterSlave<Master>::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}


template<bool Master>
void MasterSlave<Master>::RaiseError(int Errno, const char *Additional) const
{
	std::ostringstream ss;
	
	/* TODO: Turn this debug off finally */
	if (Master && Additional) 
		ss << "Master Error: " << Additional;
	else if (Additional)
		ss << "Slave Error: " << Additional;

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

template<bool Master>
void MasterSlave<Master>::SendMessage(const std::string &Msg, int Address, int Function)
{
	Lower.SendMessage(Msg, Address, Function);
}

/************************************
 * Callbacks for lowlevel interface
 * and for timeout.
 ************************************/
template<bool Master>
MasterSlave<Master>::LowerCB::LowerCB(MasterSlave<Master> &MM) : M(MM)
{
}

template<bool Master>
void MasterSlave<Master>::LowerCB::ReceivedByte(char Byte)
{
	if (M.HigherCB)
		M.HigherCB->ReceivedByte(Byte);
}

template<bool Master>
void MasterSlave<Master>::LowerCB::SentByte(char Byte)
{
	if (M.HigherCB)
		M.HigherCB->SentByte(Byte);
}

template<bool Master>
void MasterSlave<Master>::LowerCB::SentMessage(int Address, int Function, const std::string &Msg)
{
	std::cerr << "Not implemented" << std::endl;
	if (M.HigherCB)
		M.HigherCB->SentMessage(Address, Function, Msg);
}

template<bool Master>
void MasterSlave<Master>::LowerCB::ReceivedMessage(int Address, int Function, const std::string &Msg)
{
	std::cerr << "Not implemented" << std::endl;
	if (M.HigherCB)
		M.HigherCB->ReceivedMessage(Address, Function, Msg);
}

template<bool Master>
void MasterSlave<Master>::LowerCB::Error(int Errno, const char *Description)
{
	std::cerr << "Got error from lower layer: "
		  << Errno 
		  << std::endl;
	if (M.HigherCB) {
		/* Pass this error to interface with callback */
		M.HigherCB->Error(Errno, Description);
	}
}

template<bool Master>
MasterSlave<Master>::TimeoutCB::TimeoutCB(MasterSlave<Master> &MM) : M(MM)
{
}

template<bool Master>
void MasterSlave<Master>::TimeoutCB::Run()
{
	Notice = 1;
	std::cout << "Not implemented" << std::endl;
}

/**@{ Explicit template specialization */
template class MasterSlave<true>;
template class MasterSlave<false>;
/*@}*/
