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
#ifndef _LOWLEVEL_H_
#define _LOWLEVEL_H_

#include <string>

/** Interface of all low-level communication methods 
 * - most obviously the interface of a serial.
 *
 * \brief Implementation must be interrupt/signal based. Because of 
 * this we will have to inform higher levels of incoming data.
 * As a callback we will define an abstract class which will be 
 * implemented by higher layer and passed to the initializer of our class.
 *
 */
class Lowlevel
{
public:

	/** Callback interface declaration */
	class Callback
	{
	public:
		/** Default virtual constructor */
		virtual ~Callback();

		/** Called when we receive a single byte. */
		virtual void ReceivedByte(char Byte) = 0;

		/** Called when we send a single byte. */
		virtual void SentByte(char Byte) = 0;

		/** Called on any error; to be defined */
		virtual void Error(int Errno) = 0;
	};

	/* Must be virtual */
	virtual ~Lowlevel();
	
	/** Registers higher-level (usually middle) callback */
	virtual void RegisterCallback(Callback *HigherCB) = 0;

	/*** Low-level interface ***/
	/** Function initializes transmission of a single byte.
	 * It might block until the send buffer is empty */
	virtual void SendByte(char Byte) = 0;

	/** If there's a byte waiting in a queue - return it, 
	 * otherwise returns -1. */
	virtual int GetByte() = 0;

	/*** High-level interface ***/
	/** Non-Blocking function which initializes transmission of 
	 * the data stored in passed buffer. Buffer must continue
	 * to exist until all data is sent.
	 *
	 * Function might block only if the previous buffer is still
	 * being send, not for the whole transmission period.
	 *
	 * We can use std::vector instead of std::string; or 
	 * even char* + size or our own class, as you wish. */
	virtual void SendString(const std::string &Buffer);
};

#endif
