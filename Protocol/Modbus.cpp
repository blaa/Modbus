#include "Modbus.h"

Modbus::Modbus(Callback &CB, Lowlevel &LL) : C(CB), L(LL)
{
}

void Modbus::RegisterCallback(Callback &C)
{
	this->C = C;
}


void Modbus::SendMessage(const std::string &Msg, int Address)
{
	/* Form new buffer with modbus thingies and pass to serial */
}
