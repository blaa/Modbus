#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "Protocol.h"
#include "Lowlevel/Lowlevel.h"


/** Implements modbus ascii protocol */
class Modbus : public Protocol
{
protected:
	/** Callback passed to Modbus from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *C;

	/** Lowlevel class created and configured in Interface
	 * We store it, and pass it our callback. */
	Lowlevel &L;

	/* Lowlevel callback implementation */
	class LowlevelCallback : public Lowlevel::Callback
	{
		Modbus &M;
	public:
		LowlevelCallback(Modbus &MM);

		/** Called when we receive a single byte. */
		virtual void ByteReceived(char Byte);
		/** Called on any error; to be defined */
		virtual void Error(int Errno);
	};

	void ByteReceived(char Byte);


public:
	Modbus(Callback *CB, Lowlevel &LL);
	virtual void RegisterCallback(Callback *C);
	virtual void SendMessage(const std::string &Msg, int Address = 0);

};

#endif
