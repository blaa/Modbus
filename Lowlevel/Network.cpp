#include "Config.h"

#if NETWORK

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include "Network.h"

/* POSIX headers */
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>



Network *CurrentNet;

void NetworkSignalHandler(int sig, siginfo_t *sigi, void *arg)
{
	if (!CurrentNet) {
		std::cerr << "Got network signal but no handler installed - ignoring" 
			  << std::endl;
		return;
	}

	CurrentNet->SignalHandler(sigi->si_fd);
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

	if (sigaction(SIGRTMIN, &sa, NULL) < 0) {
		std::cerr << "Network, sigaction: " << strerror(errno)
			  << std::endl;
		throw -1;		
	}

	/* Ignore SIGPIPE - so we can close clients cleanly */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGPIPE, &sa, NULL) < 0) {
		std::cerr << "Network, sigaction2: " << strerror(errno)
			  << std::endl;
		throw -1;		
	}

	/* Change to asynchronous */
//	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
//	fcntl(Socket, F_SETOWN, getpid());
//	fcntl(Socket, F_SETSIG, SIGRTMIN); /* Dont't send SIGIO, but SIGRTMIN */

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
