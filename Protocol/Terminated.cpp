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
		       Lowlevel &Lower, int Timeout,
		       const std::string &Terminator)
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
	Lower.SendString(Msg + Terminator);
}

void Terminated::Ping()
{
	WaitForPing = true;
	Lower.SendString(std::string("PING") + Terminator);
	Timeout::Register(this, this->Timeout);
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

void Terminated::Accept()
{
	/** FIXME - couldn't it be more optimal? O(n) */
	Buffer.erase(Received - Terminator.size(), Buffer.size());
//	std::string Msg = Buffer.substr(0, Received - Terminator.size());
 
	if (WaitForPing) {
		WaitForPing = false;

		/* Show data as it came */
		HigherCB->ReceivedMessage(Buffer, -1, -1);

		/* Raise error to inform about ping */
		if (Buffer == "PING") {
			RaiseError(Error::PONG, "Got ping reply");
		} else {
			RaiseError(Error::FRAME, "Illegal ping reply");
		}
	} else {
		if (HigherCB)
			HigherCB->ReceivedMessage(Buffer, -1, -1);
	}
}


/************************************
 * Callbacks for lowlevel interface
 * and for timeout.
 ************************************/
void Terminated::ReceivedByte(char Byte)
{
	Timeout::Register(this, this->Timeout);

	Buffer += Byte;
	Received++;

	const unsigned int TerminatorSize = Terminator.size();

	if (TerminatorSize == 0) {
		return; /* We will have to accept in timeout */
	}

	/* Can there be a terminator? */
	if (Received >= TerminatorSize) {
		if (Buffer.find(Terminator, Received - TerminatorSize) 
		    == std::string::npos) {
			/* No terminator found - return */
			std::cerr << "No terminator found '"
				  << Buffer 
				  << "'" << std::endl;
			return;
		}
	}

	std::cerr << "Terminator!" << std::endl;

	Accept();
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
	/* Timeout! */
	if (Terminator.size() == 0 && Received > 0) {
		/* No terminator - accept as a frame */
		Accept();
		return;
	} else {
		Reset();
	}

	if (WaitForPing) {
		WaitForPing = false;
		RaiseError(Error::TIMEOUT, "While waiting for ping reply");
	} else {
		RaiseError(Error::TIMEOUT);
	}
}
