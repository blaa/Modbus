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
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <string>

/** Interface of the middle layer - modbus/terminated/other protocols */
class Protocol
{
public:
	/** Callback class which should be implemented by 
	 * user interface and configured in protocol object. */
	class Callback
	{
	public:
		/** Default virtual constructor */
		virtual ~Callback() {}

		/** For single bytes; for showing all transmitted data */
		virtual void ReceivedByte(char Byte) = 0;

		/** For single bytes; for showing all transmitted data */
		virtual void SentByte(char Byte) = 0;

		/** Informs about new arrived message */
		virtual void ReceivedMessage(const std::string &Msg, int Address, int Function) = 0;

		/** Informs about sent message */
		virtual void SentMessage(const std::string &Msg, int Address, int Function) = 0;

		/** Error which happened lower */
		virtual void Error(int Errno, const char *Description) = 0;
	};

	virtual ~Protocol() {}

	/** Function registering a callback */
	virtual void RegisterCallback(Callback *HigherCB) = 0;

	/** Blocking function formatting a buffer to send via lowlevel.
	 * It formats whole frame and passes to another layer, 
	 * then returns without waiting for transmission end.
	 * In protocols supporting Address it's used for 
	 * determining message destination 
	 */
	virtual void SendMessage(const std::string &Msg,
				 int Address = 0,
				 int Function = 0) = 0;
};

#endif
