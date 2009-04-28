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
#include "Error.h"

const char *Error::StrError(int Errno)
{
	switch (Errno) {
	case OK: return "No error";
	case HASH: return "Hash (CRC/LRC) error";
	case FRAME: return "Frame error";
	case TIMEOUT: return "Timeout error";
	default: return "Unknown error";
	}
}


