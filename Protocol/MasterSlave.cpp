#include <iostream>
#include <iomanip> /* std::setw std::setfill*/
#include <sstream>
#include <cctype>
#include "Utils/Error.h"
#include "Utils/Hash.h"
#include "Protocol.h"
#include "MasterSlave.h"

/************************************
 * Main modbus ascii class implementation 
 ************************************/
template<bool Master>
MasterSlave<Master>::MasterSlave(Protocol::Callback *HigherCB, Protocol &Lower, int Timeout) 
  : HigherCB(HigherCB), Lower(Lower), LowerCB(*this), TimeoutCB(*this)
{
	/* Register us in Lowlevel interface */
	Lower.RegisterCallback(&LowerCB);
	this->Timeout = Timeout;
	Reset();
}

template<bool Master>
MasterSlave<Master>::~MasterSlave()
{
	/* Deregister our callback */
	Lower.RegisterCallback(NULL);
}

template<bool Master>
void MasterSlave<Master>::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}


template<bool Master>
void MasterSlave<Master>::Reset()
{
	Timeout::Register(NULL, 0); /* Disable our previous timeout */
}


template<bool Master>
void MasterSlave<Master>::RaiseError(int Errno, const char *Additional) const
{
	/* Turn off timeout - no frame incoming */
	Timeout::Register(NULL, this->Timeout);


	/* TODO: Turn this debug off finally */
	if (Master) 
		std::cerr << "Master Error: ";
	else 
		std::cerr << "Slave Error: ";

	std::cerr << Errno 
		  << " : "
		  << Error::StrError(Errno)
		  << std::endl;
	if (Additional) {
		std::cerr << Additional << std::endl;
	}

	if (HigherCB) {
		HigherCB->Error(Errno);
		return;
	}
}

template<bool Master>
void MasterSlave<Master>::SendMessage(const std::string &Msg, int Address, int Function)
{
	std::cout << "Unimplemented" << std::endl;
}


/************************************
 * Callbacks for lowlevel interface
 * and for timeout.
 ************************************/
template<bool Master>
MasterSlave<Master>::LowerCB::LowerCB(MasterSlave<Master> &MM) : M(MM)
{
}

template<bool Master>
void MasterSlave<Master>::LowerCB::ReceivedByte(char Byte)
{
	std::cout << "Not implemented... "
		  << "shall we pass it to the higher level? Ok..." 
		  << std::endl;
	if (M.HigherCB)
		M.HigherCB->ReceivedByte(Byte);
}

template<bool Master>
void MasterSlave<Master>::LowerCB::ReceivedMessage(int Address, int Function, const std::string &Msg)
{
	std::cerr << "Not implemented" << std::endl;
}


template<bool Master>
void MasterSlave<Master>::LowerCB::Error(int Errno)
{
	std::cerr << "Got error from lower layer: "
		  << Errno 
		  << std::endl;
	if (M.HigherCB) {
		/* Pass this error to interface with callback */
		M.HigherCB->Error(Errno);
	}
}

template<bool Master>
MasterSlave<Master>::TimeoutCB::TimeoutCB(MasterSlave<Master> &MM) : M(MM)
{
}

template<bool Master>
void MasterSlave<Master>::TimeoutCB::Run()
{
	Notice = 1;
	std::cout << "Not implemented" << std::endl;
}


/**@{ Explicit template specialization */
template class MasterSlave<true>;
template class MasterSlave<false>;
/*@}*/
