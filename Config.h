#ifndef _CONFIG_H_
#define _CONFIG_H_


struct Config
{
	enum BaudRate
	{
		B150,	B200,
		B300,	B600,
		B1200,	B1800,
		B2400,	B4800,
		B9600,	B19200,
		B38400,	B57600,
		B115200, 
		B230400
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
		CS8, CS5
	};

	enum Protocol
	{
		MODBUS_ASCII,
		MODBUS_RTU,
		TERMINATED
	};

	static enum BaudRate BaudRate;
	static enum Parity Parity;
	static enum StopBits StopBits;
	static enum CharSize CharSize;
	static enum Protocol Protocol;
};

#endif
