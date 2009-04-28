#include "Config.h"

#if NETWORK

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <netdb.h> /* Resolver */

#include "NetworkUDP.h"

/************************
 * Server implementation 
 ************************/
void NetworkUDPServer::SignalHandler(int Sock)
{
	char Buff[255];
	/* Got some bytes from this fd probably */
	int Cnt;
	bool Error = true;

	struct sockaddr_in sa;
	socklen_t Length = sizeof(sa);
	/* FIXME: Rewrite to use bigger buffer */
	while ((Cnt = recvfrom(Sock, Buff, sizeof(Buff), 0, (struct sockaddr*)&sa, &Length)) > 0) {
		if (HigherCB) {
			for (int i=0; i<Cnt; i++)
				HigherCB->ReceivedByte(Buff[i]);
		}
		Error = false;

		/* Check if we remember this guy - if not store him */
		bool Found = false;
		for (std::vector<struct sockaddr_in>::iterator i = Clients.begin();
		     i != Clients.end(); i++) {
			if (i->sin_addr.s_addr == sa.sin_addr.s_addr
			    && i->sin_port == sa.sin_port) {
				Found = true;
				continue;
			}
			/* It's not him - echo current character there! */
			struct sockaddr *send_sa = (struct sockaddr*) &(*i);
			if (sendto(Socket, Buff, Cnt, 0, send_sa, sizeof(sa)) != 1) {
				std::cerr << "NetworkUDPServer, sendto, signalhandler: "
					  << strerror(errno)
					  << std::endl;
			}
		}
		if (!Found) {
			Clients.push_back(sa);
		}
	}
	if (Error) {
		std::cerr << "Got error " << strerror(errno) << std::endl;
	}
}

NetworkUDPServer::NetworkUDPServer(int Port)
{
	Socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (Socket < 0) {
		std::cerr << "NetworkUDPServer, socket: " << strerror(errno)
			  << std::endl;
		throw -1;
	}

	{
		struct sockaddr_in sa;
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons(Port);
		
		if (bind(Socket, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
			std::cerr << "NetworkUDPServer, bind: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}
	}

	/* Change to asynchronous */
	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
	fcntl(Socket, F_SETOWN, getpid());
	fcntl(Socket, F_SETSIG, SIGRTMIN); /* Dont't send SIGIO, but SIGRTMIN */
}

NetworkUDPServer::~NetworkUDPServer()
{
	close(Socket);
}

void NetworkUDPServer::SendByte(char Byte)
{
	for (std::vector<struct sockaddr_in>::iterator i = Clients.begin();
	     i != Clients.end(); i++) {
		int result;
		const struct sockaddr *sa = (const sockaddr *) &(*i);
		if (sendto(Socket, &Byte, 1, 0, sa, sizeof(struct sockaddr_in)) != 1) {
			std::cerr << "NetworkUDPServer, sendto: "
				  << strerror(errno) 
				  << std::endl;
		}
	}

	if (HigherCB) {
		HigherCB->SentByte(Byte);
	}
}

int NetworkUDPServer::GetByte()
{
	return 0;
}



/*****************************
 * Client functionality 
 *****************************/
void NetworkUDPClient::SignalHandler(int Sock)
{
	char Buff[255];
	int Cnt;
	bool Error = true;

	struct sockaddr_in sa;
	socklen_t Length = sizeof(sa);
	/* FIXME: Rewrite to use bigger buffer */
	while ((Cnt = recvfrom(Sock, Buff, sizeof(Buff), 0, (struct sockaddr*)&sa, &Length)) > 0) {
		/* Check if received from MASTER! */
		if (Server.sin_addr.s_addr == sa.sin_addr.s_addr
		    && Server.sin_port == sa.sin_port) {
			std::cerr << "NetworkUDPClient, Got some data on socket not from master"
				  << std::endl;
			return;
		}
		
		if (HigherCB) {
			for (int i=0; i<Cnt; i++)
				HigherCB->ReceivedByte(Buff[i]);
		}
		Error = false;

		/* Check if we remember this guy - if not store him */
		bool Found = false;
		for (std::vector<struct sockaddr_in>::iterator i = Clients.begin();
		     i != Clients.end(); i++) {
			/* It's not him - echo current character there! */
			struct sockaddr *send_sa = (struct sockaddr*) &(*i);
			if (sendto(Socket, Buff, Cnt, 0, send_sa, sizeof(sa)) != 1) {
				std::cerr << "NetworkUDPServer, sendto, signalhandler: "
					  << strerror(errno)
					  << std::endl;
			}
		}
		if (!Found) {
			Clients.push_back(sa);
		}
	}
	if (Error) {
		std::cerr << "Got error " << strerror(errno) << std::endl;
	}
}
}

NetworkUDPClient::NetworkUDPClient(const char *Host, int Port)
{
	Socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (Socket < 0) {
		std::cerr << "NetworkUDPClient, socket: " << strerror(errno)
			  << std::endl;
		throw -1;
	}

	{
		struct sockaddr_in sa;
		struct hostent *HostData;
		HostData = gethostbyname(Host);
		if (!HostData) {
			std::cerr << "NetworkUDPClient, gethostbyname: " << strerror(errno)
				  << std::endl;
			std::cerr << "While resolving " << Host << std::endl;
			throw -1;
		}

		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = *(unsigned long *)HostData->h_addr_list[0];
		sa.sin_port = htons(Port);
		
		if (connect(Socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
			std::cerr << "NetworkUDPClient, connect: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}
	}

	/* Change to asynchronous */
	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
	fcntl(Socket, F_SETOWN, getpid());
	fcntl(Socket, F_SETSIG, SIGRTMIN); /* Dont't send SIGIO, but SIGRTMIN */

	Connected = true;
}

NetworkUDPClient::~NetworkUDPClient()
{
	close(Socket);
}

void NetworkUDPClient::SendByte(char Byte)
{
	int result;
	if (!Connected) 
		return;
	do {
		result = write(Socket, &Byte, 1);
		fsync(Socket);
	} while (result != 1 && (errno == EAGAIN || errno == EWOULDBLOCK));
	
	if (result != 1) {
		/* Error happened - disconnect us! */
		close(Socket);
		Connected = false;
		throw -1;
	}

	if (HigherCB) {
		HigherCB->SentByte(Byte);
	}
}

int NetworkUDPClient::GetByte()
{
	std::cerr << "Not implemented" << std::endl;
	return 0;
}


#endif /* NETWORK */
