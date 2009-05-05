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


/** Implements simple line-based ascii protocol
 */
class Terminated : public Protocol, public Lowlevel::Callback, public Timeout::Callback
{
private:
	/** How many bytes are received already? */
	int Received;

	/** Buffer for frame contents */
	std::string Buffer;

	/** Terminator to be added to frame */
	std::string Terminator;
protected:
	/** Callback passed from a higher layer. */
	Protocol::Callback *HigherCB;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

	/** Lowlevel class created and configured in Interface.
	 * We store it, and pass it our callback. */
	Lowlevel &Lower;

	/** Timeout after which we will reset receiver */
	int Timeout;

public:
	/** Initialize terminated middle-layer with callback to interface (CB)
	 * and with some Lowlevel implementation 
	 * 
	 * Timeout is time in miliseconds after which we reset receiver
	 * if no new bytes came
	 */
	Terminated(Protocol::Callback *HigherCB, Lowlevel &Lower, int Timeout = 1000);

	/** Deregisters modbus protocol in lowlevel layer */
	~Terminated();

	/** Timeout interface */
	virtual void Run();
	
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Protocol::Callback *HigherCB);

	/** Invoked by interface; creates terminated frame and sends it 
	 * Address and function are ignored */
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


#endif
