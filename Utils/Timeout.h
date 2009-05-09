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
#ifdef QT_INTERFACE

/** Timeout class done with QT Interface 
 * \brief
 * It wraps QTimers which must run in main thread,
 * not worker - posix signal - thread.
 * Functions of this class musn't be run in main GUI 
 * thread - this can be fixed, but isn't required currently 
 */

class Comm;

class Timeout
{
	int TimerID;
	bool Active;
	Comm *C;
public:
	/** Inictialize timeout */
	Timeout();
	
	/** Virtual destructor - might be called 
	 * while GUI thread is locked and cannot handle 
	 * signals */
	virtual ~Timeout();

	/** Call Run() after MSec miliseconds */
	void Schedule(long MSec);

	/** Disable current timeout */
	void StopTime();

	/** Checks if timeout is active */
	bool IsActive();

	virtual void Run() = 0;

public:
	/** Called when timeout occurs */
	void RunWrapper();
};

#else /* QT_INTERFACE */

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
#endif /* QT_INTERFACE */

/** Call before using timeout. Might be required in DOS */
void TimeoutInit();


#endif /* _TIMEOUT_H_ */
