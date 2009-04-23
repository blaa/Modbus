#include <string>
#include "Lowlevel.h"


void Lowlevel::SendString(const std::string &Buffer)
{
	/* By default pass all bytes to SendByte */
	for (std::string::const_iterator i = Buffer.begin();
	     i != Buffer.end();
	     i++) {
		this->SendByte(*i);
	}
}
