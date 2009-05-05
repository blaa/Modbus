#ifndef _CONFIG_H_
#define _CONFIG_H_

/** Compile Linux-only extensions? */
#define SYS_LINUX	1
/** Compile Dos-only extensions? */
#define SYS_DOS		0
/** Compile network lowlevel? (BSD Sockets) */
#define NETWORK		1

/** Structure holding all configuration variables */
struct Config
{
	/*** Lowlevel configuration ***/
	/** Possible baudrates */
	enum BaudRate
	{
		BR150=0,	BR200,
		BR300,		BR600,
		BR1200,		BR1800,
		BR2400,		BR4800,
		BR9600,		BR19200,
		BR38400,	BR57600,
		BR115200,	BR230400
	};

	/** Serial parity */
	enum Parity
	{
		NONE = 0, EVEN, ODD
	};

	/** Serial stop bits */
	enum StopBits
	{
		SINGLE, DOUBLE
	};

	/** Serial character size (7-8 bits should be usable) */
	enum CharSize
	{
		CharSize8, CharSize7, CharSize5
	};

	/** Serial flow control */
	enum FlowControl
	{
		FLOWNONE = 0, RTSCTS, XONXOFF
	};
};

#endif
