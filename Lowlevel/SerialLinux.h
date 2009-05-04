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
#ifndef _SERIALLINUX_H_
#define _SERIALLINUX_H_

#include "Config.h"
#if SYS_LINUX
#include "Lowlevel.h"

/** Class implementing lowlevel interface using serial port. */
class Serial : public Lowlevel
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *HigherCB;

	/** Device descriptor */
	int fd;
public:
	/** Initialize serial interface with following middle-layer 
	 * callback and specified serial configuration */
	Serial(enum Config::BaudRate BR = Config::BR115200,
	       enum Config::Parity P = Config::EVEN,
	       enum Config::StopBits SB = Config::SINGLE,
	       enum Config::CharSize CS = Config::CharSize8, 
	       enum Config::FlowControl FC = Config::FLOWNONE, 
	       const char *Device="/dev/ttyS0");
	
	/** Deinitialize device */
	~Serial();

	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();

	/** Register new callback to middle-layer */
	virtual void RegisterCallback(Callback *HigherCB);
};

#endif /* SYS_LINUX */
#endif /* _SERIALLINUX_H_ */
