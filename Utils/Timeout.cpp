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
#include "Utils/Error.h"
#include "Lowlevel/Safe.h"

#if SYS_LINUX
#	include <cstdlib>
#	include <cstring>
#	include <unistd.h>
#	include <signal.h>
#	include <sys/time.h>
#endif


#if 0 /* OLD Linux signal-based timeout! */
#if SYS_LINUX

/** Callback which will be called */
Callback *CurrentCB;

/** Definition of notice variable */
volatile unsigned char Notice;

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
	CurrentCB = NULL;
	if (CB) {
		std::cout << "Real Timeout!!!" << std::endl;
		CB->Run(); /* This may set another Callback! */
	} else {
		std::cout << "Empty Timeout!!!" << std::endl;
	}
	Notice = 1;
}

/** Function registers callback */
void Register(Callback *CB, long MSec)
{
	struct itimerval itv;
	CurrentCB = NULL;

	Notice = 0;

	if (MSec == 0) {
		CurrentCB = NULL;
	} else {
		CurrentCB = CB;
	}
	std::cout << "Waiting max for " << MSec << std::endl;
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

#endif /* SYS_LINUX */
#endif /* 0 */

#if QT_INTERFACE
#include "Interface/ModbusFrame.h"

Timeout::Timeout()
{
	C = dynamic_cast<Comm *>(QThread::currentThread());
	if (!C) {
		std::cerr << "What did you do to the currentThread?!"
			  << std::endl;
		throw Error::Exception("Trying to create timeout outside of Comm thread");
	}
	TimerID = C->TimerRegister(this);
	Active = false;
}

Timeout::~Timeout()
{
/*	Comm *C = dynamic_cast<Comm *>(QThread::currentThread());
	if (!C) {
		std::cerr << "What did you do to the currentThread?!"
			  << std::endl;
		throw Error::Exception("Trying to create timeout outside of Comm thread");
		} */
//	C->TimerFree(TimerID);
}

void Timeout::RunWrapper()
{
	/* Add mutex security */
	Mutex::Safe();
	if (!Active) { /** This is mystery. - Qt Queue loop problem? */
		std::cerr << "Timeout::RunWrapper - NOT ACTIVE! END!" << std::endl; 
		Mutex::Unsafe();
		return;
	}
	Active = false;
	Run();
	Mutex::Unsafe();
}

void Timeout::Schedule(long MSec)
{
	Active = true;
	Comm *C = dynamic_cast<Comm *>(QThread::currentThread());
	if (!C) {
		if (!this->C) {
			std::cerr << "OK!!! THAT'S ENOUGH!"<< std::endl;
			throw Error::Exception("That shouldn't happen! FIXME! It happened once!");
		}
		/* Ok, we are called from GUI thread*/
		this->C->TimerStartSlot(TimerID, MSec);
		return;
/*		std::cerr << "What did you do to the currentThread?!"
			  << std::endl;
			  throw Error::Exception("Trying to create timeout outside of Comm thread"); */
	}
	C->TimerStart(TimerID, MSec);
}

void Timeout::StopTime()
{
	if (!Active) {
		return;
	}

	Comm *C = dynamic_cast<Comm *>(QThread::currentThread());
	if (!C) {
		if (!this->C) {
			std::cerr << "OK!!! THAT'S ENOUGH2!"<< std::endl;
			throw Error::Exception("That shouldn't happen2! FIXME! It happened once!");
		}
		/* Ok, we are called from GUI thread*/
		std::cout << " from GUI thread" << std::endl;
		this->C->TimerStopSlot(TimerID);
		return;
	}
	C->TimerStop(TimerID);
	Active = false;
}

bool Timeout::IsActive()
{
	return Active;
}

/** For a certain, locked x miliseconds wait time */
class MiliTimeout : public Timeout
{
public:
	/** 'Ready' marker */
	volatile unsigned char Set;

	MiliTimeout() : Set(0)
	{
	}

	/** Just mark that it's ready */
	virtual void Run()
	{
		Set = 1;
	}
};


void TimeoutInit()
{
}

/*
  void Sleep(long MSec)
  {
  if (MSec == 0)
  return;
  MiliTimeout MCB;
  Register(&MCB, MSec);
  while (!MCB.Set);
  }

  void Wait()
  {
  std::cerr << "Timeout::Wait() NOT IMPLEMENTED" << std::endl;
  }
*/
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


