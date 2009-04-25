#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Select one operating system */
#define SYS_LINUX	1
#define SYS_DOS		0

/** Structure holding all configuration variables */
struct Config
{
	/*** Lowlevel configuration ***/
	/** Possible baudrates */
	enum BaudRate
	{
		BR150,		BR200,
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
		NONE, EVEN, ODD
	};

	enum StopBits
	{
		SINGLE, DOUBLE
	};

	enum CharSize 
	{
		CharSize8, CharSize5
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
