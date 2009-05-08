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

#ifndef _TIMEOUT_H_
#define _TIMEOUT_H_

#ifdef QT_INTERFACE
#include <QtCore/QObject>
#include <QtCore/QTimer>
#endif

/** Functions which allows allocation of a one callback
 * which is to be called in a certain period of time
 * in future - OS dependant */

	/** Timeout callback class */
#if QT_INTERFACE
class Timeout : private QTimer
{
	Q_OBJECT

	void RunWrapper();

public:
	/** Inictialize timeout */
	Timeout();
	
	/** Virtual destructor */
	virtual ~Timeout() {}

	/** Call Run() after MSec miliseconds */
	void Schedule(long MSec, bool Periodic = false);

	/** Disable current timeout */
	void StopTime();

	/** Checks if timeout is active */
	bool IsActive();
public slots:
	/** Called when timeout occurs */
	virtual void Run() = 0;
};

#else

class Timeout
{
public:
	/** Virtual destructor */
	virtual ~Timeout() {}

	void Schedule(long MSec);
	void StopTime();

	/** Called when timeout occurs */
	virtual void Run() = 0;
};
#endif

/** Call before using timeout. Might be required in DOS */
void TimeoutInit();

#endif
