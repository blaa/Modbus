#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "Config.h"
#if NETWORK

#include <vector>

#include "Lowlevel.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* System dependent: */
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


/** Class implementing a lowlevel network interface - server side. */
class NetworkServer : public Lowlevel
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *HigherCB;

	/**@{Network state variables */
	int Socket;
	std::vector<int> Clients;
	/*@}*/

public:
	/** Initialize network with specified configuration */
	NetworkServer(int Port = 5000);

	/* Deregister us, and close sockets */
	~NetworkServer();

	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();

	/** Register new callback to middle-layer */
	virtual void RegisterCallback(Callback *C);

	friend void NetworkServerSignalHandler(int sig, siginfo_t *sigi, void *arg);

};

/** Class implementing a lowlevel network interface - client side. */
class NetworkClient : public Lowlevel
{
protected:
	/** Callback to middle-layer */
	Lowlevel::Callback *HigherCB;

	/**@{Network state variables */
	int Socket;
	bool Connected;
	/*@}*/

public:
	/** Initialize network with specified configuration */
	NetworkClient(const char *Host = "localhost", int Port = 5000);

	/* Deregister us, and close sockets */
	~NetworkClient();

	/** Send a single byte over RS232 */
	virtual void SendByte(char Byte);

	/** Block until a single byte might be read */
	virtual int GetByte();

	/** Register new callback to middle-layer */
	virtual void RegisterCallback(Callback *C);

	friend void NetworkClientSignalHandler(int sig, siginfo_t *sigi, void *arg);

};



#endif /* NETWORK */
#endif /* _NETWORK_H_ */
