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
#include <cerrno>
#include "Utils/Error.h"
#include "Utils/Hash.h"
#include "Protocol.h"
#include "MasterSlave.h"

#if SYS_LINUX
/* For some slave functions */
#include <time.h> /* Time functions */
#include <stdio.h> /* popen */
#endif

/************************************
 * General master/slave functions
 ************************************/
MasterSlave::MasterSlave(Protocol::Callback *HigherCB,
			 Protocol &Lower)
	: HigherCB(HigherCB), Lower(Lower)
{
	/* Register us in lower interface */
	Lower.RegisterCallback(this);
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
	if (HigherCB) {
		if (Additional)
			HigherCB->Error(Errno, Additional);
		else
			HigherCB->Error(Errno, NULL);
	}
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
	if (HigherCB)
		HigherCB->SentMessage(Msg, Address, Function);
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

/****************************
 * Master implementation 
 ***************************/

Master::Master(Protocol::Callback *HigherCB,
	       Protocol &Lower, 
	       int Retries,
	       long TransactionTimeout)
	: MasterSlave(HigherCB, Lower), Retries(Retries), TransactionTimeout(TransactionTimeout)
{
	Transaction = false;
	TransactionRetries = 0;
}

void Master::SendMessage(const std::string &Msg, int Address, int Function)
{
	if (Address != 0) {
		Transaction = true;
		TransactionBody = Msg;
		TransactionAddress = Address;
		TransactionFunction = Function;
		TransactionRetries = Retries - 1;
	}

	Lower.SendMessage(Msg, Address, Function);

	if (Address != 0)
		Schedule(TransactionTimeout);
}


void Master::ReceivedMessage(const std::string &Msg, int Address, int Function)
{
	if (HigherCB)
		HigherCB->ReceivedMessage(Msg, Address, Function);

	if (Transaction) {
		Transaction = false;
		if (Address == TransactionAddress) {
			/* Got reply! */
			StopTime();
		} else {
			RaiseError(Error::RESPONSE, "Wrong slave address");
		}
	}
}

void Master::Run()
{
	if (!Transaction)
		return;

	/* Ok. Transaction timed out. */
	if (TransactionRetries <= 0) {
		Transaction = false;
		RaiseError(Error::TRANSACTION);
		return;
	}
	TransactionRetries--;
	Lower.SendMessage(TransactionBody, TransactionAddress, TransactionFunction);
	Schedule(TransactionTimeout);
}


/****************************
 * Slave implementation 
 ***************************/
Slave::Slave(Protocol::Callback *HigherCB,
	     Protocol &Lower, 
	     int Address)
	: MasterSlave(HigherCB, Lower), Address(Address)
{
	/* Turn off functions */
	FunEcho = FunTime = FunText = FunExec = FunString = -1;
}

void Slave::SendMessage(const std::string &Msg, int Address, int Function)
{
	Lower.SendMessage(Msg, this->Address, Function);
}

void Slave::ReceivedMessage(const std::string &Msg, int Address, int Function)
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

	if (Function == FunString) {
		StringFunction(Msg, Address);
		return;
	}

	/* This return value; execute only if not broadcasted! */
	if (Address == 0)
		return;
	
	if (Function == FunEcho) {
		Lower.SendMessage(Msg, Address, Function);
	} else if (Function == FunTime) {
		TimeFunction();
	} else if (Function == FunText) {
		Lower.SendMessage(this->Reply, Address, Function);
	} else if (Function == FunExec) {
		ExecFunction();
	}
}

void Slave::StringFunction(const std::string &Msg, int Address)
{
	if (Address == this->Address) {
		/* Not broadcast */
		Lower.SendMessage("", this->Address, FunString & 0xFE);
	}

	RaiseError(Error::STRINGFUN, Msg.c_str());
}

void Slave::TimeFunction()
{
	char Buffer[100];
	time_t t;
	struct tm *tmp;
	int Function = FunTime;
	t = time(NULL);
	tmp = localtime(&t);
	if (!tmp) { 
		strncpy(Buffer, strerror(errno), sizeof(Buffer));
		Function |= 0x01;
	} else {
		if (strftime(Buffer, sizeof(Buffer), "%F %T", tmp) <= 0) {
			strncpy(Buffer, strerror(errno), sizeof(Buffer));
		}
		Function &= 0xFE;
	}
	Lower.SendMessage(Buffer, Address, Function);
}

void Slave::ExecFunction()
{
	char Buffer[300];
	FILE *f = popen(this->Cmd.c_str(), "r");
	if (!f) {
		strcpy(Buffer, "Execution error");
		Lower.SendMessage(Buffer, Address, FunExec | 0x01);
		return;
	}
	
	if (fgets(Buffer, sizeof(Buffer), f) <= 0) {
		strcpy(Buffer, "Read error");
		Lower.SendMessage(Buffer, Address, FunExec | 0x01);
		fclose(f);
		return;
	}
	fclose(f);
	Lower.SendMessage(Buffer, Address, FunExec & 0xFE);
}

void Slave::EnableEcho(int Function)
{
	FunEcho = Function;
}

void Slave::EnableTime(int Function)
{
	FunTime = Function;
}

void Slave::EnableString(int Function)
{
	FunString = Function;
}

void Slave::EnableText(int Function, const std::string &Reply)
{
	FunText = Function;
	this->Reply = Reply;
}

void Slave::EnableExec(int Function, const std::string &Cmd)
{
	FunExec = Function;
	this->Cmd = Cmd;
}
