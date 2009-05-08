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

#include <iostream>
#include "Safe.h"

#if QT_INTERFACE
#include <QtCore/QMutex>
extern QMutex SafeMutex;
#endif

namespace Mutex {
	/** Enable safe mode */
	void Safe()
	{
#if QT_INTERFACE
		std::cerr << "Locking safe" << std::endl;
		SafeMutex.lock();
#endif
	}

	/** Disable safe mode */
	void Unsafe()
	{
#if QT_INTERFACE
		std::cerr << "Unlocking safe" << std::endl;
		SafeMutex.unlock();
#endif
	}

};
