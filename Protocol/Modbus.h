#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "Protocol.h"

class Modbus : public Protocol
{
protected:
	/** Callback passed to Modbus from interface 
	 * will inform user about new messages 
	 */
	Protocol::Callback &C;
	
public:
	Modbus(Callback &C);
	virtual void RegisterCallback(Callback &C);
	virtual void SendMessage(const std::string &Msg, int Address = 0);

};

#endif
