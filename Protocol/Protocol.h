#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <string>

/** Interface of the middle layer - modbus/terminated/other protocols */
class Protocol
{
public:
	/** Callback class which should be implemented by 
	 * user interface and configured in protocol object. */
	class Callback
	{
	public:
		/** For single bytes; for showing all transmitted data */
		virtual void ReceivedByte(char Byte) = 0;
		virtual void ReceivedMessage(const std::string &Msg, int Address = 0) = 0;

		/* Error which happened lower */
		virtual void Error(int Errno) = 0;
	};

	/** Function registering a callback */
	virtual void RegisterCallback(Callback *C) = 0;

	/** Blocking function formatting a buffer to send via lowlevel.
	 * It formats whole frame and passes to another layer, 
	 * then returns without waiting for transmission end.
	 * In protocols supporting Address it's used for 
	 * determining message destination 
	 */
	virtual void SendMessage(const std::string &Msg, int Address = 0) = 0;
};

#endif
