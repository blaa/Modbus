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
		virtual void Run() = 0;
	};

	/** Make the system call CB->Callback after Sec seconds and MSec miliseconds */
	void Register(Callback *CB, long Sec, long MSec);
	
	/** Initialize timeout */
	void Init();
};


#endif
