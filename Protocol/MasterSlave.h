#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "Utils/Hash.h"
#include "Utils/Timeout.h"
#include "Protocol.h"

/** Implements Master/Slave protocol above another middle layer 
 * Supports:
 * Master resend; respond error, timeout error
 * Slave - few functions builtin (ping)
 */
template<bool Master>
class MasterSlave : public Protocol
{
private:

protected:
	/** This will inform us about what happened lower */
	class LowerCB : public Protocol::Callback
	{
		/** Modbus instance which needs to be informed */
		MasterSlave<Master> &M;

		/** Private constructor; only M/S class can create an instance */
		LowerCB(MasterSlave<Master> &MM);
	public:
		/** For single bytes; for showing all transmitted data */
		virtual void ReceivedByte(char Byte);

		/** Informs about new arrived message */
		virtual void ReceivedMessage(int Address, int Function, const std::string &Msg);

		/** Error which happened lower */
		virtual void Error(int Errno);

		friend class MasterSlave<Master>;
	};

	class TimeoutCB : public Timeout::Callback
	{
		/** Modbus instance which needs to be informed */
		MasterSlave<Master> &M;

		/** Set to 1 after timeout */
		volatile unsigned char Notice;

		/* Private constructor like with LL callback */
		TimeoutCB(MasterSlave &M);
	public:
		virtual void Run();

		friend class MasterSlave<Master>;
	};

	/** Callback passed to Modbus from interface 
	 * we will inform user about new messages 
	 * by calling it's methods.
	 */
	Protocol::Callback *HigherCB;

	/** Lower layer; (middle-level protocol)
	 * We store it, and pass it our callback. */
	Protocol &Lower;

	/** Instance of callback which will be passed down */
	LowerCB LowerCB;

	/** Instance of timeout callback */
	TimeoutCB TimeoutCB;

	/** Timeout after which we will reset receiver */
	int Timeout;

	/** Function raising an error */
	void RaiseError(int Errno, const char *Additional = NULL) const;

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
	MasterSlave(Protocol::Callback *HigherCB, Protocol &Lower, int Timeout = 1000);

	/** Deregisters modbus protocol in lowlevel layer */
	~MasterSlave();
	
	/** Register new callback to higher interface */
	virtual void RegisterCallback(Protocol::Callback *HigherCB);

	/** Invoked by interface; creates a frame and sends it; waits for reply if master */
	virtual void SendMessage(const std::string &Msg, int Address = 0, int Function = 0);

	/** Resets state */
	void Reset();
};

/* Explicit specializations of MasterSlave */
/* Modbus ASCII */
typedef MasterSlave<true> Master;
typedef MasterSlave<false> Slave;

#endif