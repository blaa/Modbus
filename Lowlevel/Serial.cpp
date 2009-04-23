#include <iostream>
#include "Serial.h"

Serial::Serial(Callback *CB) : C(CB)
{
	/* Type here some stuff */
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

