#include <iostream>
#include "Config.h"

#include "Lowlevel/Lowlevel.h"
#include "Protocol/Modbus.h"

namespace Testcase {

	class SimuSerial : public Lowlevel {
	protected:
		Callback *CB;
	public:
		SimuSerial()
		{
			CB = NULL;
		}
		
		/* Normal functions */
		virtual void SendByte(char Byte)
		{
			std::cout << "Serial transmit '"
				  << std::hex << Byte << std::dec << "'"
				  << std::endl;
		}

		virtual int GetByte()
		{
			int a = 'X';
			std::cout << "Serial receive norm '"
				  << std::hex << a << std::dec << "'"
				  << std::endl;
			return a;
		}

		virtual void RegisterCallback(Callback *CB) 
		{
			this->CB = CB;
		}

		/* Simulation functions */
		/* Simulate incoming byte */
		void InterruptIncomming(char Byte)
		{
			std::cout << "Serial receive inter '"
				  << std::hex << Byte << std::dec << "'"
				  << std::endl;
			
			if (this->CB) {
				CB->ByteReceived(Byte);
			}
		}

		void SimuError(int Errno)
		{
			if (this->CB) {
				CB->Error(Errno);
			}
		}

	};

	/* This is what should interface implement */
	class InterfaceCallback : public Protocol::Callback
	{
		virtual void ReceivedByte(char Byte)
		{
			std::cout << "Interface got byte from middle '"
				  << std::hex << Byte << std::dec << "'"
				  << std::endl;
		}

		virtual void ReceivedMessage(const std::string &Msg, int Address = 0)
		{
			std::cout << "Interface got correct frame from middle" 
				  << std::endl;
			std::cout << std::hex;
			for (std::string::const_iterator i = Msg.begin();
			     i != Msg.end();
			     i++) {
				std::cout << "0x" << *i << " ";
			}
			std::cout << std::dec << std::endl;
		}
		
		virtual void Error(int Errno)
		{
			std::cout << "Interface received error " 
				  << Errno << std::endl;

		}
	};

	void Middle()
	{
		/* Simulate what should interface do */
		InterfaceCallback InterfaceCallback;
		SimuSerial LowlevelLayer;
		Modbus(&InterfaceCallback, LowlevelLayer);
		
	}


























};


int main(void)
{
	/* Test middle-level protocol */
	Testcase::Middle();
	return 0;
}
