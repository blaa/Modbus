#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "Lowlevel.h"

/** Class implementing lowlevel interface using serial port. */
class Serial : public Lowlevel
{
protected:
	Lowlevel::Callback &C;

public:
	Serial(Callback &CB /*, ... Shall take configuration parameters
			      and initialize serial port */);

	virtual void SendByte(char Byte);

	virtual int GetByte();
	virtual void SendString(const std::string Buffer);


	virtual void RegisterCallback(Callback &C);
};


#endif
