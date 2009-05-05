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

/** Implements Master/Slave protocol above another middle layer 
 * Supports:
 * Master resend; respond error, timeout error
 * Slave - few functions builtin (ping)
 */

class MasterSlave : public Protocol, public Protocol::Callback
{
protected:
	/** Transaction timeout */
	class TimeoutCB : public Timeout::Callback
	{
		/** Modbus instance which needs to be informed */
		MasterSlave &M;

		/** Set to 1 after timeout */
		volatile unsigned char Notice;

		/* Private constructor like with LL callback */
		TimeoutCB(MasterSlave &M);
	public:
		virtual void Run();

		friend class MasterSlave;
	};

	/** Callback passed to Modbus from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *HigherCB;

	/** Lower layer; (middle-level protocol)
	 * We store it, and pass it our callback. */
	Protocol &Lower;

	/** Instance of timeout callback */
	TimeoutCB TimeoutCB;

	/** Timeout after which we will reset receiver */
	int Timeout;

	/** Our address */
	int Address;

	/** Do not pass informations up */
	bool Quiet;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

public:
	/** Initialize middle-layer with callback to interface HigherCB
	 * and with some lower protocol (modbus). 
	 */
	MasterSlave(Protocol::Callback *HigherCB, Protocol &Lower, int Address, int Timeout = 1000);

	/** Deregisters modbus protocol in lowlevel layer */
	~MasterSlave();

	/** Resets state */
	void Reset();

	/**@{Protocol interface} */
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Protocol::Callback *HigherCB);

	/** Invoked by interface; invokes sending of a frame in lower protocol
	 * then if master - waits for reply if master */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);

	/**@}*/

	/**@{ Callback interface */
	/** For single bytes; for showing all transmitted data */
	virtual void ReceivedByte(char Byte);

	/** For single bytes; for showing all transmitted data */
	virtual void SentByte(char Byte);

	/** Informs about new arrived message */
	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function);

	/** Informs about new sent message */
	virtual void SentMessage(const std::string &Msg, int Address, int Function);

	/** Error which happened lower */
	virtual void Error(int Errno, const char *Description);
	/**@}*/
};

/* Explicit specializations of MasterSlave */
/* Modbus ASCII */
typedef MasterSlave Master;
typedef MasterSlave Slave;

#endif
