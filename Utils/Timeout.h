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

/** Functions which allows allocation of a one callback
 * which is to be called in a certain period of time
 * in future - OS dependant */
namespace Timeout {
	/** Timeout callback class */
	class Callback
	{
	public:
		/** Virtual destructor */
		virtual ~Callback() {}
		
		/** Called when timeout occurs */
		virtual void Run() = 0;
	};

	/** Set to 1 on each timeout, and cleared with Register() */
	extern volatile unsigned char Notice;

	/** Make the system call CB->Callback after MSec miliseconds.
	 * MSec = 0 disables previous timeout */
	void Register(Callback *CB, long MSec);
	
	/** Locked wait of certain time 
	 * This may remove registered timeout. 
	 */
	void Sleep(long MSec);

	/** Wait until timeout occurs - if any defined */
	void Wait();

	/** Initialize timeout */
	void Init();
};


#endif
