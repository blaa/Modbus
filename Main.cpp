#include <iostream>
#include <cstring>
#include "Config.h"

#include "Lowlevel/Lowlevel.h"
#include "Protocol/Modbus.h"

namespace Testcase {
	/** Class simulating a working lowlevel implementation 
	 * for testing middle and interface layers */
	class SimuSerial : public Lowlevel {
	protected:
		/** Middle-layer callback */
		Callback *CB;
	public:
		SimuSerial()
		{
			CB = NULL;
		}

		void ShowByte(unsigned char Byte)
		{
			if (Byte != '\n' && Byte != '\r') {
				std::cout << "'" << Byte << "'"
					  << " (0x" << std::hex
					  << (unsigned int)Byte << ")" << std::dec;
			} else {
				if (Byte == '\n')
					std::cout << "'\\n'";
				else
					std::cout << "'\\r'";
			}
		}
		
		/** Shows what would be sent */
		virtual void SendByte(char Byte)
		{
			std::cout << "Serial transmit ";
			ShowByte(Byte);
			std::cout << std::endl;
		}
	       
		/** Simulate retrieval of single byte; returns always 'X' */
		virtual int GetByte()
		{
			int a = 'X';
			std::cout << "Serial receive norm ";
			ShowByte(a);
			std::cout << std::endl;
			return a;
		}

		/** Register new middle-layer callback */
		virtual void RegisterCallback(Callback *CB) 
		{
			this->CB = CB;
		}

		/* Simulation functions */
		/** Simulate incoming byte */
		void InterruptIncoming(unsigned char Byte)
		{
			std::cout << "Serial receive inter ";
			ShowByte(Byte);
			std::cout << std::endl;

			if (this->CB) {
				CB->ByteReceived(Byte);
			}
		}

		/** Simulate error */
		void SimuError(int Errno)
		{
			if (this->CB) {
				CB->Error(Errno);
			}
		}

	};

	/** This is what should interface implement */
	class InterfaceCallback : public Protocol::Callback
	{
		virtual void ReceivedByte(char Byte)
		{
			std::cout << "Interface got byte from middle '"
				  << std::hex << Byte << std::dec << "'"
				  << std::endl;
		}

		virtual void ReceivedMessage(int Address, int Function, const std::string &Msg)
		{
			std::cout << "Interface got correct frame from middle. Addr=" 
				  << Address
				  << " Func="
				  << Function
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
		Modbus M(&InterfaceCallback, LowlevelLayer);
		
		const char *CorrectFrames[] = {
			":000148454C4C4FAF\r\n",
			NULL
		};

		const char *IncorrectFrames[] = {
			"4FFFFF\r\n", /* Wrong start */
			":AAAABBBB\nFFFF\r\n", /* Wrong LF inside */
			":AAAABBBB\rFFFF\r\n", /* Wrong CR inside */
			":0000000148454C4C4FFFFF\r\n", /* Wrong CRC */
			NULL
		};

		/* TODO: Substitute \xFF\xFF with CRC */

		/* Simulate incoming correct frames */
		for (unsigned int y=0; CorrectFrames[y] != NULL; y++) {
			const char *Frame = CorrectFrames[y];
			std::cout << "Simulating correct frame " << y << std::endl;
			M.Reset();
			for (unsigned int i=0; i<strlen(Frame); i++) {
				LowlevelLayer.InterruptIncoming(Frame[i]);
			}
		}
		
		/* Simulate incoming correct frames */
		for (unsigned int y=0; IncorrectFrames[y] != NULL; y++) {
			const char *Frame = IncorrectFrames[y];
			std::cout 
				<< std::endl
				<< std::endl
				<< "Simulating incorrect frame " << y << std::endl;
			M.Reset();
			for (unsigned int i=0; i<strlen(Frame); i++) {
				LowlevelLayer.InterruptIncoming(Frame[i]);
			}
		}

	}
};


int main(void)
{
	/* Test middle-level protocol */
	Testcase::Middle();
	return 0;
}
