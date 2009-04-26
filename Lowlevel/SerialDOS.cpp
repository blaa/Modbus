#include "Config.h"

#if SYS_DOS

#include <iostream>
#include "SerialDOS.h"

Serial::Serial(Callback *HigherCB) : HigherCB(HigherCB)
{
	/* Type here some stuff */
	std::cerr << "Unimplemented" << std::endl;
}

void Serial::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}

void Serial::SendByte(char Byte)
{

}

int Serial::GetByte()
{
	return 0;
}

#endif /* SYS_DOS */
