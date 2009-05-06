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
#ifndef _MASTERSLAVE_H_
#define _MASTERSLAVE_H_

#include "Utils/Hash.h"
#include "Utils/Timeout.h"
#include "Protocol.h"

/** Implements a general set of functions for Master/Slave protocol
 * working above another middle level layer (like modbus)
 */
class MasterSlave : public Protocol, public Protocol::Callback
{
protected:
	/** Callback passed to Masterslave from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *HigherCB;

	/** Lower layer; (middle-level protocol)
	 * We store it, and pass it our callback. */
	Protocol &Lower;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

public:
	/** Initialize middle-layer with callback to interface HigherCB
	 * and with some lower protocol (modbus). 
	 */
	MasterSlave(Protocol::Callback *HigherCB, Protocol &Lower);

	/** Deregisters modbus protocol in lowlevel layer */
	~MasterSlave();

	/**@{Protocol interface} */

	/** Register new callback to higher interface */
	virtual void RegisterCallback(Protocol::Callback *HigherCB);

	/** Implemented in Master and Slave */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0) = 0;

	/**@}*/

	/**@{ Protocol callback interface */
	/** For single bytes; for showing all transmitted data */
	virtual void ReceivedByte(char Byte);

	/** For single bytes; for showing all transmitted data */
	virtual void SentByte(char Byte);

	/** Informs about new arrived message - this is still pure virtual */
	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function) = 0;

	/** Informs about new sent message */
	virtual void SentMessage(const std::string &Msg, int Address, int Function);

	/** Error which happened lower */
	virtual void Error(int Errno, const char *Description);
	/**@}*/
};


class Master : public MasterSlave, public Timeout
{
	/** Transaction timeout */
	class TimeoutCB : public Timeout
	{
		/** Master instance which needs to be informed */
		Master &M;

		/** Set to 1 after timeout */
		volatile unsigned char Notice;

		/* Private constructor like with LL callback */
		TimeoutCB(Master &M);
	public:


		friend class Master;
	};

//	TimeoutCB TimeoutCB;

	int Retries;
	int TransactionTimeout;
public:
	Master(Protocol::Callback *HigherCB, Protocol &Lower, int Retries = 0, int TransactionTimeout = 0);
	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function);
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);

	/** Timeout interface */
	virtual void Run();
};

class Slave : public MasterSlave
{
	int Address;

public:
	Slave(Protocol::Callback *HigherCB, Protocol &Lower, int Address);
	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function);
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);
};



#endif
