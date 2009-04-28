/**********************************************************************
 * Comm -- Connection framework
 * (C) 2009 by Tomasz bla Fortuna <bla@thera.be>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * See Docs/LICENSE
 *********************/
#include "Config.h"

#if NETWORK

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <netdb.h> /* Resolver */

#include "Utils/Error.h"
#include "NetworkTCP.h"

/************************
 * Server implementation 
 ************************/
void NetworkTCPServer::SignalHandler(int Sock)
{
	if (Sock == Socket) {
		/* Main socket */
		int Client = accept(Sock, NULL, NULL);
		if (Client < 0) {
			std::cerr << "NetworkTCPServerSignalHandler, accept: "
				  << strerror(errno) 
				  << std::endl;
			return;
		}
		std::cout << "CLIENT ACCEPTED!!!" << std::endl;
		Clients.push_back(Client);

		fcntl(Client, F_SETFL, O_ASYNC | O_NONBLOCK);
		fcntl(Client, F_SETOWN, getpid());
		fcntl(Client, F_SETSIG, SIGRTMIN);
	} else {
		/* Got some bytes from this fd probably */
		int Cnt;
		char Ch;
		bool Error = true;
		while ((Cnt = read(Sock, &Ch, 1)) == 1) {
			if (HigherCB) {
				HigherCB->ReceivedByte(Ch);
			}
			Error = false;
		}
		if (Error) {
			/* Client will get erased on first write() */
			/* Don't access vector in signal */
		}
	}
}

NetworkTCPServer::NetworkTCPServer(int Port)
{
	Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket < 0) {
		std::cerr << "NetworkTCPServer, socket: " << strerror(errno)
			  << std::endl;
		throw Error::Exception("NetworkTCPServer, socket: ", strerror(errno));
	}

	{
		struct sockaddr_in sa;
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons(Port);
		
		if (bind(Socket, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
			std::cerr << "NetworkTCPServer, bind: " << strerror(errno)
				  << std::endl;
			throw Error::Exception("NetworkTCPServer, bind: ", strerror(errno));
		}

		if (listen(Socket, 10) < 0) {
			std::cerr << "NetworkTCPServer, listen: " << strerror(errno)
				  << std::endl;
			throw Error::Exception("NetworkTCPServer, listen: ", strerror(errno));
		}
	}

	/* Change to asynchronous */
	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
	fcntl(Socket, F_SETOWN, getpid());
	fcntl(Socket, F_SETSIG, SIGRTMIN); /* Dont't send SIGIO, but SIGRTMIN */
}

NetworkTCPServer::~NetworkTCPServer()
{
	close(Socket);
	for (std::vector<int>::iterator i = Clients.begin();
	     i != Clients.end();
	     i++) {
		close(*i);
	}
}

void NetworkTCPServer::SendByte(char Byte)
{
	for (std::vector<int>::iterator i = Clients.begin();
	     i != Clients.end();) {
		int result;
		do {
			result = write(*i, &Byte, 1);
			fsync(*i);
		} while (result != 1 && (errno == EAGAIN || errno == EWOULDBLOCK));

		if (result != 1) {
			/* Error happened - disconnect client */
			close(*i);
			i = this->Clients.erase(i);
		} else {
			i++;
		}
	}

	if (HigherCB) {
		HigherCB->SentByte(Byte);
	}
}

int NetworkTCPServer::GetByte()
{
	return 0;
}



/*****************************
 * Client functionality 
 *****************************/
void NetworkTCPClient::SignalHandler(int Sock)
{
	if (Sock != Socket) {
		std::cerr << "Something is rotten" << std::endl;
		return;
	}

	/* Got some bytes from this fd */
	int Cnt;
	char Ch;
	bool Error = true;
	while ((Cnt = read(Sock, &Ch, 1)) == 1) {
		if (HigherCB) {
			HigherCB->ReceivedByte(Ch);
		}
		Error = false;
	}
	if (Error) {
		/* Client will get erased on first write() */
		/* Don't access vector in signal */
	}
}

NetworkTCPClient::NetworkTCPClient(const char *Host, int Port)
{
	Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket < 0) {
		std::cerr << "NetworkTCPClient, socket: " << strerror(errno)
			  << std::endl;
		throw Error::Exception("NetworkTCPClient, socket: ", strerror(errno));
	}

	{
		struct sockaddr_in sa;
		struct hostent *HostData;
		HostData = gethostbyname(Host);
		if (!HostData) {
			std::cerr << "NetworkTCPClient, gethostbyname: " << strerror(errno)
				  << std::endl;
			std::cerr << "While resolving " << Host << std::endl;
			throw Error::Exception("NetworkTCPClient, gethostbyname: ", strerror(errno));
		}

		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = *(unsigned long *)HostData->h_addr_list[0];
		sa.sin_port = htons(Port);
		
		if (connect(Socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
			std::cerr << "NetworkTCPClient, connect: " << strerror(errno)
				  << std::endl;
			throw Error::Exception("NetworkTCPClient, connect: ", strerror(errno));
		}
	}

	/* Change to asynchronous */
	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
	fcntl(Socket, F_SETOWN, getpid());
	fcntl(Socket, F_SETSIG, SIGRTMIN); /* Dont't send SIGIO, but SIGRTMIN */

	Connected = true;
}

NetworkTCPClient::~NetworkTCPClient()
{
	close(Socket);
}

void NetworkTCPClient::SendByte(char Byte)
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
		throw Error::Exception("NetworkTCPClient, disconnected from master: ",
				       strerror(errno));
	}

	if (HigherCB) {
		HigherCB->SentByte(Byte);
	}
}

int NetworkTCPClient::GetByte()
{
	std::cerr << "Not implemented" << std::endl;
	return 0;
}


#endif /* NETWORK */
