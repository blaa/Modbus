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

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "Config.h"
#if NETWORK

#include <vector>
#include <signal.h> /* siginfo_t */

#include "Lowlevel.h"

/** Base class for implementing networking */
class Network : public Lowlevel
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *HigherCB;

	/** Handle system signals regarding network */
	virtual void SignalHandler(int fd) = 0;

public:
	/** Register signal handler */
	Network();

	/* Deregister signal handler */
	~Network();

	/* Will call HandleSignal() on signal... */
	friend void NetworkSignalHandler(int sig, siginfo_t *sigi, void *arg);

	/** Register new callback to middle-layer */
	virtual void RegisterCallback(Callback *C);
};

#endif /* NETWORK */
#endif /* _NETWORK_H_ */
