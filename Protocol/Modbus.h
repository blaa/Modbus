#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "Utils/CRC.h"
#include "Protocol.h"
#include "Lowlevel/Lowlevel.h"


/** Implements modbus ascii protocol 
 *
 * \brief
 * This class implements the MODBUS ASCII protocol
 * as defined by MODICON. In future it may also handle
 * RTU version.
 *
 */
class Modbus : public Protocol
{
private:
	/** How many bytes are received already? */
	int Received;

	/** Buffer for frame contents */
	std::string Buffer;

	/** Slave address */
	unsigned char Address;

	/** Invoked function */
	unsigned char Function;

	/** CRC calculation */
	LRC CRC;

protected:
	/** Callback passed to Modbus from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *C;

	/** Function raising an error */
	void RaiseError(int Errno);

	/** Lowlevel class created and configured in Interface
	 * We store it, and pass it our callback. */
	Lowlevel &L;

	/** Lowlevel callback implementation */
	class LowlevelCallback : public Lowlevel::Callback
	{
		/** Modbus instance which needs to be informed */
		Modbus &M;

		/** Private constructor; only Modbus class can create
		 * an instance */
		LowlevelCallback(Modbus &MM);
	public:

		/** Called when we receive a single byte. */
		virtual void ByteReceived(char Byte);
		/** Called on any error; to be defined */
		virtual void Error(int Errno);
	};

	/** This collects bytes into frames */
	void ByteReceived(char Byte);


public:
	/** Initialize modbus middle-layer with callback to interface (CB)
	 * and with some Lowlevel implementation */
	Modbus(Callback *CB, Lowlevel &LL);
	
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Callback *C);

	/** Invoked by interface; creates modbus frame and sends it */
	virtual void SendMessage(const std::string &Msg, int Address = 0);

};

#endif
