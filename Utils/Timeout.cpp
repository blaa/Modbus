#include <iostream>

#include "Config.h"
#include "Timeout.h"

#if SYS_LINUX
#	include <cstdlib>
#	include <cstring>
#	include <signal.h>
#	include <sys/time.h>
#endif

namespace Timeout {

	/** Callback which will be called */
	Callback *CurrentCB;
	
#if SYS_LINUX
	
	/** Linux Signal handler */
	void Handler(int Flag, siginfo_t *si, void *Arg)
	{
		Callback *CB = CurrentCB;
		if (CB)
			CB->Run();
		CurrentCB = 0;
	}


	/** Function registers callback */
	void Register(Callback *CB, long Sec, long MSec)
	{
		struct itimerval itv;
		memset(&itv, 0, sizeof(itv));
		itv.it_value.tv_sec = Sec;
		itv.it_value.tv_usec = MSec * 1000;
		CurrentCB = CB;
		if (setitimer(ITIMER_REAL, &itv, NULL) != 0) {
			perror("setitimer");
			exit(-1);
		}
		/* Will deliver SIGALRM */
	}

	/** Initializes Linux signals for Timeout*/
	void Init()
	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = NULL;
		sa.sa_sigaction = Handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_SIGINFO;
		sa.sa_restorer = NULL;
		
		CurrentCB = NULL;
		if (sigaction(SIGALRM, &sa, NULL) != 0) {
			perror("sigaction");
			exit(-1);
		}

	}
#endif


#if SYS_DOS
	/** To be written; Dos version of Register function */
	void Register(Callback *CB, long Sec, long MSec)
	{
		CurrentCB = CB;
		std::cerr << "Unimplemented!" << std::endl;
	}

	void Init()
	{
		std::cerr << "Unimplemented!" << std::endl;
	}
#endif


};
