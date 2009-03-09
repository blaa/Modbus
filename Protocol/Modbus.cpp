#include "Modbus.h"

Modbus(Callback &CB) : C(CB)
{
}

void RegisterCallback(Callback &C)
{
	this->C = C;
}


void Modbus::SendMessage(const std::string &Msg, int Address = 0)
{
	/* Form new buffer with modbus thingies and pass to serial */
}
