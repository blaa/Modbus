
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
