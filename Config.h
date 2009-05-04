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
		BR115200, 
		BR230400
	};

	enum Parity 
	{
		NONE = 0, EVEN, ODD
	};

	enum StopBits
	{
		SINGLE, DOUBLE
	};

	enum CharSize 
	{
		CharSize8, CharSize7, CharSize5
	};

	enum FlowControl
	{
		FLOWNONE = 0, RTSCTS, XONXOFF
	};

	/* Serial configuration */
	static enum BaudRate BaudRate;
	static enum Parity Parity;
	static enum StopBits StopBits;
	static enum CharSize CharSize;

	/*** Middle level configuration ***/
	enum Protocol
	{
		MODBUS_ASCII,
		MODBUS_RTU,
		TERMINATED
	};

	enum Role
	{
		MASTER,
		SLAVE
	};

	/* Current protocol */
	static enum Protocol Protocol;
	/* Modbus slave address */
	static unsigned char Address;
	/* Modbus role */
	static enum Role Role;
};

#endif
