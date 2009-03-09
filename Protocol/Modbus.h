#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "Protocol.h"
#include "Lowlevel/Lowlevel.h"

class Modbus : public Protocol
{
protected:
	/** Callback passed to Modbus from interface 
	 * we will inform user about new messages 
	 * via this interface.
	 */
	Protocol::Callback &C;
	Lowlevel &L;
public:
	Modbus(Callback &C, Lowlevel &LL);
	virtual void RegisterCallback(Callback &C);
	virtual void SendMessage(const std::string &Msg, int Address = 0);

};

#endif
