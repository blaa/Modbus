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
#include "Safe.h"

namespace Unix {
#include <signal.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
};


/* Hide this functions */
//namespace {
	int fd;
	Serial::Callback *CurrentCB;

	void SerialSignalHandler(int a)
	{
		/* Enter safe section - no send will be queued this way
		 * and we will wait until some send finish if in progress */
//		Mutex::Safe();

		char Ch;
		if (CurrentCB == NULL || fd <= 0)
			return;
		while (1 == read(fd, &Ch, 1)) {
			CurrentCB->ReceivedByte(Ch);
		}

//		Mutex::Unsafe();
	}
//}

Serial::Serial(enum Config::BaudRate BR, enum Config::Parity P,
	       enum Config::StopBits SB, enum Config::CharSize CS,
	       enum Config::FlowControl FC,
	       const char *Device)
{
	using namespace Unix;

	struct termios newtio;
	struct sigaction sa;

	CurrentCB = HigherCB = NULL;

	/* Set handler first */
	sa.sa_handler = SerialSignalHandler;
	sa.sa_restorer = NULL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGIO, &sa, NULL) != 0) {
		std::cerr << "sigaction: " << strerror(errno);
		throw Error::Exception("Serial, sigaction: ", strerror(errno));
	}

	/* Unblock SIGIO and SIGRTMIN */
	sigset_t ss;
	sigemptyset(&ss);
	sigaddset(&ss, SIGIO);
	sigaddset(&ss, SIGRTMIN);
	sigprocmask(SIG_UNBLOCK, &ss, NULL);

	/* Then open serial device in nonblocking/async mode */
	this->fd = open(Device, O_RDWR | O_NOCTTY | O_NONBLOCK | O_ASYNC);

	if (this->fd < 0) {
		std::cerr << "Unable to open " << Device << std::endl;
		std::cerr << "open: " << strerror(errno) << std::endl;
		throw Error::Exception("Error while opening serial: ", strerror(errno));
	}

	/* And configure it */
	speed_t Speed;
	switch (BR) {
	case Config::BR150: Speed = B150; break;
	case Config::BR200: Speed = B200; break;
	case Config::BR300: Speed = B300; break;
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
		close(this->fd);
		throw Error::Exception("Internal error while selecting serial speed");
	}

	if (tcgetattr(this->fd, &newtio) != 0) {
		std::cerr << "tcgetattr: " << strerror(errno);
		close(this->fd);
		throw Error::Exception("Error while reading serial attributes", strerror(errno));
	}

	/* Clear any unwanted */
	newtio.c_lflag = 0;
	newtio.c_cflag = 0;
	newtio.c_iflag = 0;
	newtio.c_oflag = 0;

	/* Return immediately returning what you have */
	newtio.c_cc[VMIN] = newtio.c_cc[VTIME] = 0;

	/* Speed + enable receiver */
	newtio.c_cflag = Speed | CREAD;
	cfsetispeed(&newtio, Speed);
	cfsetospeed(&newtio, Speed);

	/* Set character size */
	switch (CS) {
	case Config::CharSize5:
		newtio.c_cflag |= CS5;
		CSMask = 0xE0;
		break;
	case Config::CharSize6:
		newtio.c_cflag |= CS6;
		CSMask = 0xC0;
		break;
	case Config::CharSize7:
		newtio.c_cflag |= CS7;
		CSMask = 0x80;
		break;
	default:
		newtio.c_cflag |= CS8;
		CSMask = 0x00;
		break;
	}

	/* Set parity */
	switch (P) {
	case Config::EVEN:
		newtio.c_cflag |= INPCK | PARENB; break;
	case Config::ODD:
		newtio.c_cflag |= INPCK | PARENB | PARODD; break;
	default:
		break;
	}

	/* Two stopbits instead of one? */
	if (SB == Config::DOUBLE)
		newtio.c_cflag |= CSTOPB;

	/* Flow control */
	switch (FC) {
        case Config::RTSCTS:
		newtio.c_cflag |= CRTSCTS;
		break;
	case Config::XONXOFF:
		newtio.c_iflag |= IXON | IXOFF;
		break;
	default:
	case Config::FLOWNONE:
		newtio.c_cflag |= CLOCAL; /* Ignore control lines */
		break;
	};
    
	if (tcflush(this->fd, TCIOFLUSH) != 0) {
		std::cerr << "tcflush: " << strerror(errno);
		close(this->fd);
		throw Error::Exception("Error while flushing serial device", strerror(errno));
	}

	if (tcsetattr(this->fd, TCSANOW, &newtio) != 0) {
		std::cerr << "tcsetattr: " << strerror(errno);
		close(this->fd);
		throw Error::Exception("Error while setting serial attributes", strerror(errno));
	}

	::fd = this->fd;

	fsync(this->fd);

	fcntl(this->fd, F_SETOWN, syscall(SYS_gettid));
	fcntl(this->fd, F_SETFL, O_ASYNC);
}

Serial::~Serial()
{
	CurrentCB = NULL;
	close(this->fd);
	::fd = -1;
}

void Serial::RegisterCallback(Callback *HigherCB)
{
	this->HigherCB = HigherCB;
	CurrentCB = HigherCB;
	::fd = this->fd;
}

void Serial::SendByte(char Byte)
{
	if ((unsigned char)Byte & CSMask) {
		if (this->HigherCB)
			this->HigherCB->Error(Error::ILBYTE);
		return;
	}
	write(this->fd, &Byte, 1);
	fsync(this->fd);
	Unix::tcdrain(this->fd);
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
