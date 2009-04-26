#include <iostream>
#include <cstring>
#include "Config.h"
#include "Utils/Timeout.h"
#include "Utils/Error.h"
#include "Lowlevel/Lowlevel.h"
#include "Lowlevel/Serial.h"
#include "Protocol/Modbus.h"


namespace Testcase {
	/** Excessive debug */
	const bool ED = true;


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
			if (ED) {
				std::cout << "Serial transmit ";
				ShowByte(Byte);
				std::cout << std::endl;
			}

			/* Loop output to input */
			InterruptIncoming(Byte);
		}
	       
		/** Simulate retrieval of single byte; returns always 'X' */
		virtual int GetByte()
		{
			int a = 'X';
			if (ED) {
				std::cout << "Serial receive norm ";
				ShowByte(a);
				std::cout << std::endl;
			}
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
			if (ED) {
				std::cout << "Serial receive inter ";
				ShowByte(Byte);
				std::cout << std::endl;
			}
			
			if (this->CB) {
				CB->ReceivedByte(Byte);
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
	public:
		volatile unsigned char Timeout;

		virtual void ReceivedByte(char Byte)
		{
			std::cout << "Interface got byte from middle '"
				  << std::hex << Byte << std::dec << "'"
				  << std::endl;
		}

		virtual void ReceivedMessage(int Address, int Function, const std::string &Msg)
		{
			std::cout << "Interface got correct frame from middle. Addr=" 
				  << std::dec
				  << Address
				  << " Func="
				  << Function
				  << std::endl;
			std::cout << std::hex;
			for (std::string::const_iterator i = Msg.begin();
			     i != Msg.end();
			     i++) {
				std::cout << "0x" << (unsigned int)(unsigned char)*i << " ";
			}
			std::cout << "== '" << Msg << "'" << std::endl;
			std::cout << std::endl;
		}
		
		virtual void Error(int Errno)
		{
			std::cout << "Interface received error no " 
				  << Errno
				  << " =>'" << Error::StrError(Errno) << "'"
				  << std::endl;

			/* Warn about timeout error */
			if (Errno == Error::TIMEOUT)
				Timeout = 1;
		}
	};

	void Middle()
	{
		/* Simulate what should interface do */
		InterfaceCallback InterfaceCallback;
		SimuSerial LowlevelLayer;
		ModbusASCII M(&InterfaceCallback, LowlevelLayer);
		
		const char *CorrectFrames[] = {
			":000148454C4C4F8B\r\n",
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
		
		/* Simulate incoming incorrect frames */
		for (unsigned int y=0; IncorrectFrames[y] != NULL; y++) {
			const char *Frame = IncorrectFrames[y];
			std::cout 
				<< std::endl << std::endl
				<< "Simulating incorrect frame " << y << std::endl;
			M.Reset();
			for (unsigned int i=0; i<strlen(Frame); i++) {
				LowlevelLayer.InterruptIncoming(Frame[i]);
			}
		}

		/* Will cause hang without working TIMEOUT 
		 * Enable for DOS only when writting dos timeout */
		if (SYS_LINUX) {
			std::cout 
				<< std::endl << std::endl
				<< "Simulating timeout problem" << std::endl;
			
			/* Simulate timeout problem */ 
			const char *Frame = CorrectFrames[0];
			for (unsigned int i=0; i<strlen(Frame)/2; i++) {
				LowlevelLayer.InterruptIncoming(Frame[i]);
			}
			InterfaceCallback.Timeout = 0;
			while (InterfaceCallback.Timeout == 0);
		}

		M.Reset();
		std::cout 
			<< std::endl << std::endl
			<< "Simulating a message sending with output looped back to"
			<< " middle layer input (adr=0, fun=1, 'HELLO')" 
			<< std::endl;
		M.SendMessage("HELLO", 0, 1);


		/*** MODBUS RTU testcases */
		/* Register another callback in lowlayer */
		std::cout
			<< std::endl << std::endl
			<< "Modbus RTU testcases" << std::endl
			<< "Sending with looped output" << std::endl;
		ModbusRTU MR(&InterfaceCallback, LowlevelLayer, 200);

		MR.SendMessage("HELLO", 'A', 'F');

		/* Wait for answer */
		while (Timeout::Notice == 0);

		/* Second testcase - timeout */
		std::cout
			<< std::endl << std::endl
			<< "Timeout test - or wrong CRC RTU check" << std::endl;

		LowlevelLayer.SendString("HELLO");

		/* Wait for answer */
		while (Timeout::Notice == 0);
	}


	class TestTimeout : public Timeout::Callback {
	public:
		volatile char TimeoutDone;

		TestTimeout()
		{
			TimeoutDone = 0;
		}

		void Run() 
		{
			std::cerr << "Running!\n" << std::endl;
			TimeoutDone = 1;
		}
	};

	void Timeout()
	{
		TestTimeout TT;
		std::cerr << "Registering callback" << std::endl;
		Timeout::Register(&TT, 1500);
		std::cerr << "Waiting..." << std::endl;
		while (!TT.TimeoutDone);
		std::cerr << "One..." << std::endl;
		Timeout::Sleep(1500);
		std::cerr << "Timeout testcase finished!\n" << std::endl;

	}

	/** Lowlevel test */
	void Lowlevel()
	{
#if SYS_LINUX
		std::cout << "Initializing lowlevel" << std::endl;
		Serial LowlevelLayer;
		/* Check if we will get interrupts */
/*		std::cout << "Waiting for interrupts" << std::endl;
		for (;;) {} */

		std::cout << "Creating middle and higher layer" << std::endl;
		InterfaceCallback InterfaceCallback;
		if (0) { /* ASCII */
			ModbusASCII M(&InterfaceCallback, LowlevelLayer);
			
			std::cout << "Waiting for ascii frames" << std::endl;
			
			for (;;) {
				/* Here is some interface loop waiting for keypresses */
			}
		} else { /* RTU */
			ModbusRTU M(&InterfaceCallback, LowlevelLayer, 200);
			
			std::cout << "Waiting for rtu frames" << std::endl;
			for (;;) {
				/* Here is some interface loop waiting for keypresses */
			}

		}
#endif /* SYS_LINUX */
	}
};


int main(int argc, char **argv)
{
	/* Initialize timeout interrupt/signal */
	Timeout::Init();

	/* Test middle-level protocol */
	Testcase::Middle();

//	Testcase::Lowlevel();

	/* Test timeout */
//	Testcase::Timeout();
	return 0;
}
