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

#ifndef _SAFE_H_
#define _SAFE_H_

#include "Network.h"
#include "Serial.h"

#if SYS_LINUX
extern void SerialSignalHandler(int a);
extern void NetworkSignalHandler(int sig, siginfo_t *sigi, void *arg);
#endif

namespace Mutex {

	/** Enable safe mode */
	inline void Safe()
	{
		NetworkLock++;
		SerialLock++;
	}

	/** Disable safe mode */
	inline void Unsafe()
	{
#if SYS_LINUX
		NetworkLock--;
		if (!NetworkLock && NetworkEvent) {
			NetworkEvent = 0;
			std::cerr << "Recalling handler after leaving safe mode!" << std::endl;
			NetworkSignalHandler(0, NULL, NULL);
		}

		SerialLock--;
		if (!SerialLock && SerialEvent) {
			SerialEvent = 0;
			std::cerr << "Recalling serial handler after leaving safe mode!" << std::endl;
			SerialSignalHandler(0);
		}
#endif
	}
}

#endif
