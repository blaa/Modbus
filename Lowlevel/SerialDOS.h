#ifndef _SERIALDOS_H_
#define _SERIALDOS_H_

#include "Config.h"
#if SYS_DOS

#include "Lowlevel.h"

/** Class implementing lowlevel interface using serial port. */
class Serial : public Lowlevel
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *C;

public:
	/** Initialize serial interface with following middle-layer 
	 * callback and specified serial configuration */
	Serial(Callback *CB = NULL /*, ... Shall take configuration parameters
			      and initialize serial port */);
	
	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();

	/** Register new callback to middle-layer */
	virtual void RegisterCallback(Callback *C);
};


#endif /* SYS_DOS */
#endif /* _SERIALDOS_H_ */
