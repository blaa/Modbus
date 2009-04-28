#include "Config.h"

#if NETWORK

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <netdb.h> /* Resolver */

#include "Network.h"

/************************
 * Server implementation 
 ************************/

NetworkServer *CurrentNS;
NetworkClient *CurrentNC;

void NetworkServerSignalHandler(int sig, siginfo_t *sigi, void *arg)
{
	if (!CurrentNS)
		return;
	int Sock = sigi->si_fd;

	if (Sock == CurrentNS->Socket) {
		/* Main socket */
		int Client = accept(Sock, NULL, NULL);
		if (Client < 0) {
			std::cerr << "NetworkServerSignalHandler, accept: "
				  << strerror(errno) 
				  << std::endl;
			return;
		}
		std::cout << "CLIENT ACCEPTED!!!" << std::endl;
		CurrentNS->Clients.push_back(Client);

		fcntl(Client, F_SETFL, O_ASYNC | O_NONBLOCK);
		fcntl(Client, F_SETOWN, getpid());
		fcntl(Client, F_SETSIG, SIGRTMIN);
	} else {
		/* Got some bytes from this fd probably */
		int Cnt;
		char Ch;
		bool Error = true;
		while ((Cnt = read(Sock, &Ch, 1)) == 1) {
			if (CurrentNS->HigherCB) {
				CurrentNS->HigherCB->ReceivedByte(Ch);
			}
			Error = false;
		}
		if (Error) {
			/* Client will get erased on first write() */
			/* Don't access vector in signal */
		}
	}
}

void NetworkClientSignalHandler(int sig, siginfo_t *sigi, void *arg)
{
	if (!CurrentNC)
		return;
	int Sock = sigi->si_fd;

	if (Sock != CurrentNC->Socket) {
		std::cerr << "Something is rotten" << std::endl;
		return;
	}

	/* Got some bytes from this fd */
	int Cnt;
	char Ch;
	bool Error = true;
	while ((Cnt = read(Sock, &Ch, 1)) == 1) {
		if (CurrentNC->HigherCB) {
			CurrentNC->HigherCB->ReceivedByte(Ch);
		}
		Error = false;
	}
	if (Error) {
		/* Client will get erased on first write() */
		/* Don't access vector in signal */
	}

}


/*****************************
 * Server functionality 
 *****************************/

NetworkServer::NetworkServer(int Port)
	: HigherCB(HigherCB)
{
	Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket < 0) {
		std::cerr << "NetworkServer, socket: " << strerror(errno)
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
			std::cerr << "NetworkServer, bind: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}

		if (listen(Socket, 10) < 0) {
			std::cerr << "NetworkServer, listen: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}
	}

	/* Install signal handler */
	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_sigaction = NetworkServerSignalHandler;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);


		if (sigaction(SIGRTMIN, &sa, NULL) < 0) {
			std::cerr << "NetworkServer, sigaction: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}

		/* Ignore SIGPIPE - so we can close clients cleanly */
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);
		if (sigaction(SIGPIPE, &sa, NULL) < 0) {
			std::cerr << "NetworkServer, sigaction2: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}
	}

	/* Change to asynchronous */
	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
	fcntl(Socket, F_SETOWN, getpid());
	fcntl(Socket, F_SETSIG, SIGRTMIN); /* Dont't send SIGIO, but SIGRTMIN */

	CurrentNS = this;
}

NetworkServer::~NetworkServer()
{
	CurrentNS = NULL;
	close(Socket);
	for (std::vector<int>::iterator i = Clients.begin();
	     i != Clients.end();
	     i++) {
		close(*i);
	}
}

void NetworkServer::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}

void NetworkServer::SendByte(char Byte)
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

int NetworkServer::GetByte()
{
	return 0;
}



/*****************************
 * Client functionality 
 *****************************/
NetworkClient::NetworkClient(const char *Host, int Port)
	: HigherCB(HigherCB)
{
	Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket < 0) {
		std::cerr << "NetworkClient, socket: " << strerror(errno);
		throw -1;
	}

	{
		struct sockaddr_in sa;
		struct hostent *HostData;
		HostData = gethostbyname(Host);
		if (!HostData) {
			std::cerr << "NetworkClient, gethostbyname: " << strerror(errno)
				  << std::endl;
			std::cerr << "While resolving " << Host << std::endl;
			throw -1;
		}

		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = *(unsigned long *)HostData->h_addr_list[0];
		sa.sin_port = htons(Port);
		
/*		if (bind(Socket, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
			std::cerr << "NetworkClient, bind: " << strerror(errno);
			throw -1;		
			} */

		if (connect(Socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
			std::cerr << "NetworkClient, connect: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}
	}

	/* Install signal handler */
	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_sigaction = NetworkClientSignalHandler;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);


		if (sigaction(SIGRTMIN, &sa, NULL) < 0) {
			std::cerr << "NetworkClient, sigaction: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}

		/* Ignore SIGPIPE - so we can close clients cleanly */
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);
		if (sigaction(SIGPIPE, &sa, NULL) < 0) {
			std::cerr << "NetworkClient, sigaction2: " << strerror(errno)
				  << std::endl;
			throw -1;		
		}
	}

	/* Change to asynchronous */
	fcntl(Socket, F_SETFL, O_ASYNC | O_NONBLOCK);
	fcntl(Socket, F_SETOWN, getpid());
	fcntl(Socket, F_SETSIG, SIGRTMIN); /* Dont't send SIGIO, but SIGRTMIN */

	CurrentNC = this;
	Connected = true;
}

NetworkClient::~NetworkClient()
{
	CurrentNC = NULL;
	close(Socket);
}

void NetworkClient::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
}

void NetworkClient::SendByte(char Byte)
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

int NetworkClient::GetByte()
{
	std::cerr << "Not implemented" << std::endl;
	return 0;
}


#endif /* NETWORK */
