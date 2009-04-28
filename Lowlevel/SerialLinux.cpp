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
#if SYS_LINUX

#include <iostream>
#include <cerrno>
#include <cstring>

#include "Utils/Error.h"
#include "Lowlevel.h"
#include "SerialLinux.h"


namespace Unix {
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
};

/* Hide this functions */
namespace {
	int fd;
	Serial::Callback *CurrentCB;
	
	void SerialLinuxReceive(int a)
	{
		char Ch;

		while (1 == read(fd, &Ch, 1)) {
			if (CurrentCB) {
				CurrentCB->ReceivedByte(Ch);
			}
		}
	}
}


Serial::Serial(enum Config::BaudRate BR, enum Config::Parity P,
	       enum Config::StopBits SB, enum Config::CharSize CS,
	       const char *Device)
{
	struct Unix::termios newtio = { 0 };
	struct Unix::sigaction sa;

	this->fd = Unix::open(Device, O_RDWR | O_NOCTTY);
	::fd = this->fd;
	if (this->fd < 0) {
		std::cerr << "Unable to open " << Device << std::endl;
		std::cerr << "open: " << strerror(errno) << std::endl;
		throw Error::Exception("Error while opening serial: ", strerror(errno));
	}


	Unix::speed_t Speed;
	switch (BR) {
	case Config::BR600: Speed = B600; break;
	case Config::BR1200: Speed = B1200; break;
	case Config::BR1800: Speed = B1800; break;
	case Config::BR2400: Speed = B2400; break;
	case Config::BR4800: Speed = B4800; break;
	case Config::BR9600: Speed = B9600; break;
	case Config::BR19200: Speed = B19200; break;
	case Config::BR38400: Speed = B38400; break;
	case Config::BR57600: Speed = B57600; break;
	case Config::BR115200: Speed = B115200; break;
	case Config::BR230400: Speed = B230400; break;
	default:
		std::cerr << "Consult " << __FILE__ << ":" << __LINE__ 
			  << std::endl;
		throw Error::Exception("Internal error while selecting serial speed");
	}

	newtio.c_cflag = Speed | CREAD;

	Unix::cfsetispeed(&newtio, Speed);
	if (CS == Config::CharSize5)
		newtio.c_cflag |= CS5;
	else
		newtio.c_cflag |= CS8;

	if (P == Config::EVEN)
		newtio.c_cflag |= PARENB;

	if (SB == Config::DOUBLE)
		newtio.c_cflag |= CSTOPB;
    
	if (Unix::tcflush(this->fd, TCIFLUSH) != 0) {
		std::cerr << "tcflush: " << strerror(errno);
		throw Error::Exception("Serial, tcflush: ", strerror(errno));
	}

	if (Unix::tcsetattr(this->fd, TCSANOW, &newtio) != 0) {
		std::cerr << "tcsetattr: " << strerror(errno);
		throw Error::Exception("Serial, tcsetattr: ", strerror(errno));
	}

	
	sa.sa_handler = SerialLinuxReceive;
	sa.sa_restorer = NULL;
	Unix::sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (Unix::sigaction(SIGIO, &sa, NULL) != 0) {
		std::cerr << "sigaction: " << strerror(errno);
		throw Error::Exception("Serial, sigaction: ", strerror(errno));
	}
	
	Unix::fcntl(this->fd, F_SETOWN, getpid());
	Unix::fcntl(this->fd, F_SETFL, O_ASYNC);
	
	this->HigherCB = NULL;
	CurrentCB = NULL;
}

Serial::~Serial()
{
	close(this->fd);
}

void Serial::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
	CurrentCB = HigherCB;
	::fd = this->fd;
}

void Serial::SendByte(char Byte)
{
	write(this->fd, &Byte, 1);
	if (this->HigherCB)
		this->HigherCB->SentByte(Byte);
}

int Serial::GetByte()
{
	char Byte;
	/* This might not work in asynchronous */
	read(this->fd, &Byte, 1);
	return Byte;
}


#endif /* SYS_LINUX */
