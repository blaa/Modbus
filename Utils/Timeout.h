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
		/** Called when timeout occurs */
		virtual void Run() = 0;
	};

	/** Set to 1 on each timeout, and cleared with Register() */
	extern volatile unsigned char Notice;

	/** Make the system call CB->Callback after MSec miliseconds */
	void Register(Callback *CB, long MSec);
	
	/** Locked wait of certain time 
	 * This may remove registered timeout. 
	 */
	void Sleep(long MSec);

	/** Initialize timeout */
	void Init();
};


#endif
