#include "Config.h"

#if SYS_DOS

#include <iostream>
#include "SerialDOS.h"

Serial::Serial(Callback *CB) : C(CB)
{
	/* Type here some stuff */
	std::cerr << "Unimplemented" << std::endl;
}

void Serial::RegisterCallback(Callback *C)
{
	this->C = C;
}

void Serial::SendByte(char Byte)
{

}

int Serial::GetByte()
{
	return 0;
}

#endif /* SYS_DOS */
