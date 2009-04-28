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
#include <string>
#include "Lowlevel.h"

Lowlevel::~Lowlevel()
{
}

void Lowlevel::SendString(const std::string &Buffer)
{
	/* By default pass all bytes to SendByte */
	for (std::string::const_iterator i = Buffer.begin();
	     i != Buffer.end();
	     i++) {
		this->SendByte(*i);
	}
}
