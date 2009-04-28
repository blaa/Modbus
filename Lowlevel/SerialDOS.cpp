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
#include "Config.h"

#if SYS_DOS

#include <iostream>
#include "SerialDOS.h"

Serial::Serial(Callback *HigherCB) : HigherCB(HigherCB)
{
	/* Type here some stuff */
	std::cerr << "Unimplemented" << std::endl;
}

void Serial::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}

void Serial::SendByte(char Byte)
{
/*	if (this->HigherCB)
		this->HigherCB->SendByte(Byte); */
}

int Serial::GetByte()
{
	return 0;
}

#endif /* SYS_DOS */
