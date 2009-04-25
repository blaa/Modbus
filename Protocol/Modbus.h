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
template<typename HashType, bool ASCII>
class ModbusGeneric : public Protocol
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
	HashType Hash;

	/** Store here bytes we can't convert yet */
	char HalfByte;

protected:
	/** Lowlevel callback implementation */
	class LowlevelCallback : public Lowlevel::Callback
	{
		/** Modbus instance which needs to be informed */
		ModbusGeneric<HashType, ASCII> &M;

		/** Private constructor; only Modbus class can create
		 * an instance */
		LowlevelCallback(ModbusGeneric<HashType, ASCII> &MM);
	public:

		/** Called when we receive a single byte. */
		virtual void ByteReceived(char Byte);
		/** Called on any error; to be defined */
		virtual void Error(int Errno);

		friend class ModbusGeneric<HashType, ASCII>;
	};

	class TimeoutCallback : public Timeout::Callback
	{
		/** Modbus instance which needs to be informed */
		ModbusGeneric<HashType, ASCII> &M;

		/** Set to 1 after timeout */
		volatile unsigned char Notice;

		/* Private constructor like with LL callback */
		TimeoutCallback(ModbusGeneric &M);
	public:
		virtual void Run();

		friend class ModbusGeneric<HashType, ASCII>;
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
	static unsigned char HexConvert(unsigned char A, unsigned  char B);

public:
	/** Initialize modbus middle-layer with callback to interface (CB)
	 * and with some Lowlevel implementation 
	 * 
	 * Timeout is time in miliseconds which meaning depends on protocol type.
	 * In ASCII it is max time between two incoming characters before receiver
	 * is reset.
	 * In RTU max time between two incoming characters equals 1.5 * Timeout
	 * whereas minimal time generated after sending a frame equals 3.5 * Timeout
	 */
	ModbusGeneric(Callback *CB, Lowlevel &LL, int Timeout = 1000);

	/** Deregisters modbus protocol in lowlevel layer */
	~ModbusGeneric();
	
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Callback *C);

	/** Invoked by interface; creates modbus frame and sends it */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);
	/** Resets receiver */
	void Reset();
};

/* Explicit specializations of ModbusGeneric */
/* Modbus ASCII */
typedef ModbusGeneric<LRC, true> ModbusASCII;
typedef ModbusGeneric<CRC16, false> ModbusRTU;

#endif
