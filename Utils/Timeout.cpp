#include <iostream>

#include "Config.h"
#include "Timeout.h"

#if SYS_LINUX
#	include <cstdlib>
#	include <cstring>
#	include <unistd.h>
#	include <signal.h>
#	include <sys/time.h>
#endif

namespace Timeout {

	/** Callback which will be called */
	Callback *CurrentCB;

	/** Definition of notice variable */
	volatile unsigned char Notice;

	
#if SYS_LINUX
	
	/** For a certain, locked x miliseconds wait time */
	class MiliCallback : public Callback
	{
	public:
		volatile unsigned char Set;

		MiliCallback() : Set(0)
		{
		}

		virtual void Run()
		{
			Set = 1;
		}
	};

	/** Linux Signal handler */
	void Handler(int Flag, siginfo_t *si, void *Arg)
	{
		Callback *CB = CurrentCB;
		if (CB)
			CB->Run();
		CurrentCB = 0;
		Notice = 1;
	}


	/** Function registers callback */
	void Register(Callback *CB, long MSec)
	{
		struct itimerval itv;
		Notice = 0;
		memset(&itv, 0, sizeof(itv));
		itv.it_value.tv_sec = MSec / 1000;
		itv.it_value.tv_usec = (MSec % 1000) * 1000;
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

	void Sleep(long MSec)
	{
		MiliCallback MCB;
		Register(&MCB, MSec);
		while (!MCB.Set);
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
