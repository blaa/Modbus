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
#include "Lowlevel/Lowlevel.h"


/** Implements modbus ascii protocol 
 *
 * \brief
 * This template class implements MODBUS ASCII and RTU
 * protocols as defined by MODICON.
 *
 * It's lower layer is a Lowlevel implementation while
 * it's higher layer might be either MasterSlave or user interface.
 *
 */
template<typename HashType, bool ASCII>
class ModbusGeneric : public Protocol, public Lowlevel::Callback
{
private:
	/** How many bytes are received already? */
	int Received;

	/** Buffer for frame contents */
	std::string Buffer;

	/** Slave address */
	unsigned char Address;

	/** Invoked function */
	unsigned char Function;

	/** CRC calculation */
	HashType Hash;

	/** Store here bytes we can't convert yet */
	char HalfByte;

protected:
	class TimeoutCB : public Timeout::Callback
	{
		/** Modbus instance which needs to be informed */
		ModbusGeneric<HashType, ASCII> &M;

		/* Private constructor like with LL callback */
		TimeoutCB(ModbusGeneric &M);
	public:
		virtual void Run();

		friend class ModbusGeneric<HashType, ASCII>;
	};

	/** Callback passed to Modbus from a higher layer.
	 * We will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *HigherCB;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

	/** Lowlevel class created and configured in Interface.
	 * We store it, and pass it our callback. */
	Lowlevel &Lower;

	/** Instance of timeout callback */
	TimeoutCB TimeoutCB;

	/** Timeout after which we will reset receiver */
	int Timeout;

	/** Set after RTU frame is sent; cleared by timer */
	bool RTUGap;

	/** This collects bytes into frames; called by callback */

	/** Helper for converting ASCII hex into byte */
	static unsigned char HexConvert(unsigned char A, unsigned  char B);

public:
	/** Initialize modbus middle-layer with callback to interface (CB)
	 * and with some Lowlevel implementation 
	 * 
	 * Timeout is time in miliseconds which meaning depends on protocol type.
	 * In ASCII it is max time between two incoming characters before receiver
	 * is reset.
	 * In RTU max time between two incoming characters equals 1.5 * Timeout
	 * whereas minimal time generated after sending a frame equals 3.5 * Timeout
	 */
	ModbusGeneric(Protocol::Callback *HigherCB, Lowlevel &Lower, int Timeout = 1000);

	/** Deregisters modbus protocol in lowlevel layer */
	~ModbusGeneric();
	
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Protocol::Callback *HigherCB);

	/** Invoked by interface; creates modbus frame and sends it */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);
	/** Resets receiver */
	void Reset();

	/**@{ Protocol callback interface */
	/** Called when we receive a single byte. Collects them into a message */
	virtual void ReceivedByte(char Byte);
	
	/** Called when we send single byte.
	 * Modbus passes it higher to interface
	 * and ignores */
	virtual void SentByte(char Byte);

	/** Called on any error; to be defined */
	virtual void Error(int Errno);
	/*@}*/

};

/* Explicit specializations of ModbusGeneric */
/* Modbus ASCII */
typedef ModbusGeneric<LRC, true> ModbusASCII;
typedef ModbusGeneric<CRC16, false> ModbusRTU;

#endif
