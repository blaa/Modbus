#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "Utils/Hash.h"
#include "Utils/Timeout.h"
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
	LRC Hash;

	/* Store here bytes we can't convert yet */
	char HalfByte;

protected:
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

		friend class Modbus;
	};

	class TimeoutCallback : public Timeout::Callback
	{
		/** Modbus instance which needs to be informed */
		Modbus &M;

		/** Set to 1 after timeout */
		volatile unsigned char Notice;

		/* Private constructor like with LL callback */
		TimeoutCallback(Modbus &M);
	public:
		virtual void Run();

		friend class Modbus;
	};

	/** Callback passed to Modbus from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *C;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

	/** Lowlevel class created and configured in Interface
	 * We store it, and pass it our callback. */
	Lowlevel &L;

	/** Instance of callback which will be passed down */
	LowlevelCallback LCB;

	/** Instance of timeout callback */
	TimeoutCallback TCB;

	/** Timeout after which we will reset receiver */
	int Timeout;

	/** This collects bytes into frames; called by callback */
	void ByteReceived(char Byte);

	/** Helper for converting ASCII hex into byte */
	static unsigned char AAHexConvert(unsigned char A, unsigned  char B);

public:
	/** Initialize modbus middle-layer with callback to interface (CB)
	 * and with some Lowlevel implementation */
	Modbus(Callback *CB, Lowlevel &LL, int Timeout = 2);

	/** Deregisters modbus protocol in lowlevel layer */
	~Modbus();
	
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Callback *C);

	/** Invoked by interface; creates modbus frame and sends it */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);
	/** Resets receiver */
	void Reset();
};

#endif
