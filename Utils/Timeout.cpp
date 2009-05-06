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
		/** 'Ready' marker */
		volatile unsigned char Set;

		MiliCallback() : Set(0)
		{
		}

		/** Just mark that it's ready */
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
		std::cout << "Timeout!!!" << std::endl;
		CurrentCB = 0;
		Notice = 1;
	}

	/** Function registers callback */
	void Register(Callback *CB, long MSec)
	{
		struct itimerval itv;

		std::cout << "Waiting max for " << MSec << std::endl;
		CurrentCB = CB;
		Notice = 0;

		if (MSec == 0) {
			std::cerr << "Timeout::Register: Called with MSec = 0 - disabling"
				  << std::endl;
			CurrentCB = NULL;
			return;
		}

		memset(&itv, 0, sizeof(itv));
		itv.it_value.tv_sec = MSec / 1000;
		itv.it_value.tv_usec = (MSec % 1000) * 1000;
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
//		while (!MCB.Set);  /* If timeout gets changed we will hang; better check this:
		while (CurrentCB == &MCB);
	}
	
	void Wait()
	{
		if (!CurrentCB) {
			std::cerr << "Waiting, but no callback defined!"
				  << std::endl;
			return;
		}
		while (Notice == 0);
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
