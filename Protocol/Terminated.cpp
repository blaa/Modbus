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
#include "Lowlevel/Safe.h"

/************************************
 * Main modbus ascii class implementation 
 ************************************/

Terminated::Terminated(Protocol::Callback *HigherCB, 
		       Lowlevel &Lower, int Timeout,
		       const std::string &Terminator,
		       bool Echo)
	: Terminator(Terminator), HigherCB(HigherCB), Lower(Lower)
{
	/* Register us in Lowlevel interface */
	Lower.RegisterCallback(this);
	this->Timeout = Timeout;
	this->Echo = Echo;
	WaitForPing = false;
	Received = 0;
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
	Mutex::Safe(); /* As receving might cause another sending when echo is on
			* disable signals until we send this message */
	Lower.SendString(Msg + Terminator);
	if (HigherCB)
		HigherCB->SentMessage(Msg, Address, Function);
	Mutex::Unsafe();
}

void Terminated::Ping()
{
	WaitForPing = true;

	std::cout << "Got ping click; Setting timeout" << std::endl;
	Schedule(this->Timeout);

	SendMessage("PING", -1, -1);
}

void Terminated::Reset()
{
	StopTime(); /* Disable our previous timeout */
	Received = 0;
	Buffer.clear();
}

void Terminated::RaiseError(int Errno, const char *Additional) const
{
	std::ostringstream ss;
	if (!HigherCB)
		return;

	/* TODO: Turn this debug off finally */
	if (Additional)
		ss << "Terminated Error: " << Additional;

	std::cerr << ss.str() << std::endl;

	if (Additional)
		HigherCB->Error(Errno, ss.str().c_str());
	else
		HigherCB->Error(Errno, NULL);
}

void Terminated::Accept()
{
	StopTime(); /* Disable our previous timeout */

	/** FIXME - couldn't it be more optimal? O(n) */
	if (Terminator.size() != 0)
		Buffer.erase(Received - Terminator.size(), Terminator.size());

	/* Show data */
	if (HigherCB) 
		HigherCB->ReceivedMessage(Buffer, -1, -1);
 
	if (WaitForPing) {
		WaitForPing = false;

		/* Raise error to inform about ping */
		if (Buffer == "PING") {
			RaiseError(Error::PONG, "Got ping reply");
		} else {
			RaiseError(Error::FRAME, "Illegal ping reply");
		}
	} else {
		if (Echo)
			SendMessage(Buffer, -1, -1);
	}
	Reset();
}


/************************************
 * Callbacks for lowlevel interface
 * and for timeout.
 ************************************/
void Terminated::ReceivedByte(char Byte)
{
	if (HigherCB)
		HigherCB->ReceivedByte(Byte);

	Timeout::Schedule(this->Timeout);

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
			return;
		}
		Accept();
	} 
	/* Too short for terminator */
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
	}

	if (WaitForPing) {
		WaitForPing = false;
		RaiseError(Error::TIMEOUT, "Timeout while waiting for ping reply");
	} else {
		RaiseError(Error::TIMEOUT, "Timeout while receiving a frame");
	}
	Reset();
}
