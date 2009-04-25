#ifndef _SERIALLINUX_H_
#define _SERIALLINUX_H_

#include "Config.h"
#if SYS_LINUX
#include "Lowlevel.h"

/** Class implementing lowlevel interface using serial port. */
class Serial : public Lowlevel
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *C;

	/** Device descriptor */
	int fd;
public:
	/** Initialize serial interface with following middle-layer 
	 * callback and specified serial configuration */
	Serial(enum Config::BaudRate BR = Config::BR115200,
	       enum Config::Parity P = Config::EVEN,
	       enum Config::StopBits SB = Config::SINGLE,
	       enum Config::CharSize CS = Config::CharSize8, 
	       const char *Device="/dev/ttyS0");
	
	/** Deinitialize device */
	~Serial();

	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();

	/** Register new callback to middle-layer */
	virtual void RegisterCallback(Callback *C);
};

#endif /* SYS_LINUX */
#endif /* _SERIALLINUX_H_ */
