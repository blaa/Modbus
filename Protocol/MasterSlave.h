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
#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "Utils/Hash.h"
#include "Utils/Timeout.h"
#include "Protocol.h"

/** Implements Master/Slave protocol above another middle layer 
 * Supports:
 * Master resend; respond error, timeout error
 * Slave - few functions builtin (ping)
 */
template<bool Master>
class MasterSlave : public Protocol
{
private:

protected:
	/** This will inform us about what happened lower */
	class LowerCB : public Protocol::Callback
	{
		/** Modbus instance which needs to be informed */
		MasterSlave<Master> &M;

		/** Private constructor; only M/S class can create an instance */
		LowerCB(MasterSlave<Master> &MM);
	public:
		/** For single bytes; for showing all transmitted data */
		virtual void ReceivedByte(char Byte);

		/** For single bytes; for showing all transmitted data */
		virtual void SentByte(char Byte);

		/** Informs about new arrived message */
		virtual void ReceivedMessage(int Address, int Function, const std::string &Msg);

		/** Informs about new sent message */
		virtual void SentMessage(int Address, int Function, const std::string &Msg);

		/** Error which happened lower */
		virtual void Error(int Errno, const char *Description);

		friend class MasterSlave<Master>;
	};

	class TimeoutCB : public Timeout::Callback
	{
		/** Modbus instance which needs to be informed */
		MasterSlave<Master> &M;

		/** Set to 1 after timeout */
		volatile unsigned char Notice;

		/* Private constructor like with LL callback */
		TimeoutCB(MasterSlave &M);
	public:
		virtual void Run();

		friend class MasterSlave<Master>;
	};

	/** Callback passed to Modbus from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *HigherCB;

	/** Lower layer; (middle-level protocol)
	 * We store it, and pass it our callback. */
	Protocol &Lower;

	/** Instance of callback which will be passed down */
	LowerCB LowerCB;

	/** Instance of timeout callback */
	TimeoutCB TimeoutCB;

	/** Timeout after which we will reset receiver */
	int Timeout;

	/** Our address */
	int Address;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

public:
	/** Initialize middle-layer with callback to interface HigherCB
	 * and with some lower protocol (modbus). 
	 */
	MasterSlave(Protocol::Callback *HigherCB, Protocol &Lower, int Address, int Timeout = 1000);

	/** Deregisters modbus protocol in lowlevel layer */
	~MasterSlave();
	
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Protocol::Callback *HigherCB);

	/** Invoked by interface; invokes sending of a frame in lower protocol
	 * then if master - waits for reply if master */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);

	/** Resets state */
	void Reset();
};

/* Explicit specializations of MasterSlave */
/* Modbus ASCII */
typedef MasterSlave<true> Master;
typedef MasterSlave<false> Slave;

#endif
