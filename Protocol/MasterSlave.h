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
#ifndef _MASTERSLAVE_H_
#define _MASTERSLAVE_H_

#include "Utils/Hash.h"
#include "Utils/Timeout.h"
#include "Protocol.h"

/** Implements a general set of functions for Master/Slave protocol
 * working above another middle level layer (like modbus)
 */
class MasterSlave : public Protocol, public Protocol::Callback
{
protected:
	/** Callback passed to Masterslave from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *HigherCB;

	/** Lower layer; (middle-level protocol)
	 * We store it, and pass it our callback. */
	Protocol &Lower;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

public:
	/** Initialize middle-layer with callback to interface HigherCB
	 * and with some lower protocol (modbus). 
	 */
	MasterSlave(Protocol::Callback *HigherCB, Protocol &Lower);

	/** Deregisters modbus protocol in lowlevel layer */
	~MasterSlave();

	/**@{Protocol interface} */

	/** Register new callback to higher interface */
	virtual void RegisterCallback(Protocol::Callback *HigherCB);

	/** Implemented in Master and Slave */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0) = 0;

	/**@}*/

	/**@{ Protocol callback interface */
	/** For single bytes; for showing all transmitted data */
	virtual void ReceivedByte(char Byte);

	/** For single bytes; for showing all transmitted data */
	virtual void SentByte(char Byte);

	/** Informs about new arrived message - this is still pure virtual */
	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function) = 0;

	/** Informs about new sent message */
	virtual void SentMessage(const std::string &Msg, int Address, int Function);

	/** Error which happened lower */
	virtual void Error(int Errno, const char *Description);
	/**@}*/
};

/** Class implementing Master behaviour: Transaction and their retries */
class Master : public MasterSlave, public Timeout
{
	/** Number of retries until giving up */
	int Retries;
	/** Timeout in miliseconds before giving up */
	long TransactionTimeout;

	/**@{Transaction variables */
	/** True if there a transaction in progress */
	bool Transaction;

	/** How many tries should we wait still */
	int TransactionRetries;

	/** Waiting for reply from what slave */
	int TransactionAddress;
	/** Transaction function */
	int TransactionFunction;
	/** Message of transaction */
	std::string TransactionBody;
	/*@}*/

public:
	/** Initialize master layer and configure it with given number of retries
	 * and retry timeout. Also register in lower layer. */ 
	Master(Protocol::Callback *HigherCB, Protocol &Lower, int Retries = 0, long TransactionTimeout = 0);

	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function);
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);

	/** Timeout interface */
	virtual void Run();
};

/** Class implementing example slave behaviour:
 * Few built-in functions, replies and address filtering. */
class Slave : public MasterSlave
{
	/** Slave address */
	int Address;

	/**@{ Function parameters - numbers and arguments */
	int FunEcho;
	int FunTime;
	int FunText;
	int FunExec;
	std::string Reply;
	std::string Cmd;
	/*@} */

	/** Built-in function returning current time */
	void TimeFunction();

	/** Built-in function executing specified command */
	void ExecFunction();

public:
	/** Initialize slave layer with given address and register it 
	 * in the lower layer 
	 */
	Slave(Protocol::Callback *HigherCB, Protocol &Lower, int Address);

	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function);
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);

	/** Enable echo reply from slave */
	void EnableEcho(int Function = 1);
	/** Enable current time reply */
	void EnableTime(int Function = 2);
	/** Enable time reply */
	void EnableText(int Function = 3, const std::string &Reply = "Hello world");
	/** Enable slave function which executes program and replies output */
	void EnableExec(int Function = 4, const std::string &Cmd = "uname -a");
};



#endif
