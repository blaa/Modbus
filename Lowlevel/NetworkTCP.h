
#ifndef _NETWORKTCP_H_
#define _NETWORKTCP_H_

#include "Config.h"
#if NETWORK

#include <vector>

#include "Lowlevel.h"
#include "Network.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* System dependent: */
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


/** Class implementing a lowlevel network interface - server side. */
class NetworkTCPServer : public Network
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *HigherCB;

	/**@{Network state variables */
	int Socket;
	std::vector<int> Clients;
	/*@}*/

	virtual void SignalHandler(int Sock);
public:
	/** Initialize network with specified configuration */
	NetworkTCPServer(int Port = 5000);

	/* Deregister us, and close sockets */
	~NetworkTCPServer();

	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();
};

/** Class implementing a lowlevel network interface - client side. */
class NetworkTCPClient : public Network
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *HigherCB;

	/**@{Network state variables */
	int Socket;
	bool Connected;
	/*@}*/

	virtual void SignalHandler(int Sock);
public:
	/** Initialize network with specified configuration */
	NetworkTCPClient(const char *Host = "localhost", int Port = 5000);

	/* Deregister us, and close sockets */
	~NetworkTCPClient();

	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();
};


#endif /* NETWORK */
#endif /* _NETWORKTCP_H_ */
