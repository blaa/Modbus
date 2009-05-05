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
#include "Terminated.h"

/************************************
 * Main modbus ascii class implementation 
 ************************************/

Terminated::Terminated(Protocol::Callback *HigherCB, 
		       Lowlevel &Lower, int Timeout)
	: HigherCB(HigherCB), Lower(Lower)
{
	/* Register us in Lowlevel interface */
	Lower.RegisterCallback(this);
	this->Timeout = Timeout;
	Reset();
}


Terminated::~Terminated()
{
	/* Deregister our callback */
	Lower.RegisterCallback(NULL);
}


void Terminated::RegisterCallback(Protocol::Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}

void Terminated::SendMessage(const std::string &Msg, int Address, int Function)
{
}

void Terminated::Reset()
{
	Timeout::Register(NULL, 0); /* Disable our previous timeout */
	Received = 0;
	Buffer.clear();
}

void Terminated::RaiseError(int Errno, const char *Additional) const
{
	std::ostringstream ss;
	/* Turn off timeout - no frame incoming */
	Timeout::Register(NULL, this->Timeout);

	if (!HigherCB)
		return;

	/* TODO: Turn this debug off finally */
	if (Additional)
		ss << "Terminated Error: " << Additional;

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

/** Modbus ASCII frame grabber */
void Terminated::ReceivedByte(char Byte)
{
}

void Terminated::SentByte(char Byte)
{
	/* Inform higher layer about this single byte */
	if (HigherCB) {
		HigherCB->SentByte(Byte);
	}
}


void Terminated::Error(int Errno)
{
	std::cerr << "Got error from low layer: "
		  << Errno 
		  << std::endl;
	if (HigherCB) {
		/* Pass this error to interface with callback */
		HigherCB->Error(Errno, "Error from lowlevel");
	}
}


void Terminated::Run()
{
	/* Timeout! Reset receiver */
	Reset();
	RaiseError(Error::TIMEOUT);
}
