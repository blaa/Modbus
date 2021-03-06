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

#if NETWORK

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include "Utils/Error.h"
#include "Network.h"
#include "Safe.h"

/* POSIX headers */
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


#if QT_INTERFACE
#include <sys/syscall.h>
#include "Interface/ModbusFrame.h"
#endif

Network *CurrentNet;

/** Handle network 'interrupt' - call current network implementation */
void NetworkSignalHandler(int sig, siginfo_t *sigi, void *arg)
{
	/* Enter safe section - no send will be queued this way
	 * and we will wait until some send finish if in progress */
	if (!CurrentNet) {
		std::cerr << "Got network signal but no handler installed - ignoring" 
			  << std::endl;
		return;
	}

	Mutex::Safe();

	if (sigi)
		CurrentNet->SignalHandler(sigi->si_fd);
	else
		CurrentNet->SignalHandler(-1);

	Mutex::Unsafe();
}

/*****************************
 * Server functionality 
 *****************************/

Network::Network()
{
	HigherCB = NULL;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = NetworkSignalHandler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGNAL_ID, &sa, NULL) < 0) {
		std::cerr << "Network, sigaction: " << strerror(errno)
			  << std::endl;
		throw Error::Exception("Network, sigaction: ", strerror(errno));
	}

	/* Ignore SIGPIPE - so we can close clients cleanly in tcp/ip */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGPIPE, &sa, NULL) < 0) {
		std::cerr << "Network, sigaction2: " << strerror(errno)
			  << std::endl;
		throw Error::Exception("Network, sigaction (sigpipe): ", strerror(errno));
	}

	/* Unblock SIGIO and SIGRTMIN */
	sigset_t ss;
	sigemptyset(&ss);
	sigaddset(&ss, SIGIO);
	sigaddset(&ss, SIGRTMIN);
	sigprocmask(SIG_UNBLOCK, &ss, NULL);


	/* Change to asynchronous */
//	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
//	fcntl(Socket, F_SETOWN, getpid());
//	fcntl(Socket, F_SETSIG, SIGNAL_ID); /* Dont't send SIGIO, but SIGRTMIN */

	if (CurrentNet) {
		std::cerr << "Network, Previous network handler detected - ignoring"
			  << std::endl;
	}
	CurrentNet = this;
}

Network::~Network()
{
	CurrentNet = NULL;
}

void Network::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}


#endif /* NETWORK */
