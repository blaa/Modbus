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
#ifndef _SERIALDOS_H_
#define _SERIALDOS_H_

#include "Config.h"
#if SYS_DOS

#include "Lowlevel.h"

/** Class implementing lowlevel interface using serial port. */
class Serial : public Lowlevel
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *HigherCB;

public:
	/** Initialize serial interface with following middle-layer 
	 * callback and specified serial configuration */
	Serial(Callback *HigherCB = NULL /*, ... Shall take configuration parameters
			      and initialize serial port */);
	
	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();

	/** Register new callback to middle-layer */
	virtual void RegisterCallback(Callback *HigherCB);
};


#endif /* SYS_DOS */
#endif /* _SERIALDOS_H_ */
