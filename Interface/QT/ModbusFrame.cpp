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

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctype.h>
#include <cerrno>

#include <sys/syscall.h>
#include <QtCore/QString>
#include <QtGui/QMessageBox>

#include "ModbusFrame.h"
#include "Utils/Error.h"
#include "Lowlevel/Safe.h"

ModbusFrame::ModbusFrame(QWidget *parent)
	: QMainWindow(parent), System(parent, ui)
{
	ui.setupUi(this);

	qRegisterMetaType<QTimer *>("QTimer *");

	/** Connect signals comming from thread */
	connect(&System, SIGNAL(UpdateData(const QString &, int)),
		this, SLOT(UpdateData(const QString &, int)));

	connect(&System, SIGNAL(Status(const QString &, bool)),
		this, SLOT(Status(const QString &, bool)));

	connect(&System, SIGNAL(UpdateError(int, const char *)),
		this, SLOT(UpdateError(int, const char *)));

	/* Block SIGIO and SIGRTMIN signals by default! */
	sigset_t ss;
	sigemptyset(&ss);
	sigaddset(&ss, SIGIO);
	sigaddset(&ss, SIGRTMIN);
	sigprocmask(SIG_SETMASK, &ss, NULL);
}

ModbusFrame::~ModbusFrame()
{
	/* Schedule thread stop */
	Stop();
}

const std::string ModbusFrame::ParseEscapes(const std::string &Str)
{
	const int Length = Str.size();
	std::string New;
	for (int i=0; i<Length;) {
		if (Str[i] == '\\' && i+1 < Length) { /* \ and at least one 
							  character more */
			char Mark = Str[i+1];
			char A, B;

			switch (Mark) {

			case '\\':
				/* Omit this two */
				i += 2;
				New += '\\';
				continue;

			case 'n':
				i+=2;
				New += '\n';
				continue;

			case 'r':
				i+=2;
				New += '\r';
				continue;

			case 'x':
				if (i+3 >= Length) 
					goto copy; /* Not enough space */
				
				A = toupper(Str[i+2]);
				B = toupper(Str[i+3]);
				if (A >= '0' && A <= '9')
					A -= '0';
				else {
					if (A >= 'A' && A <= 'F')
						A = A - 'A' + 10;
					else
						goto copy; /* Wrong character */
				}
				
				if (B >= '0' && B <= '9')
					B -= '0';
				else {
					if (B >= 'A' && B <= 'F')
						B = B - 'A' + 10;
					else
						goto copy; /* Wrong character */
				}

				New += (unsigned char)(A * 16 + B);
				i += 4;
				continue;
			default:
				/* Broken, just insert it into new string */
				goto copy; 
			}
		}
	copy:
		New += Str[i];
		i++;
	}
	return New;
}

void ModbusFrame::Start()
{
	/** This could be done with QWaitCondition instead */
	std::cerr << "Scheduling initialize..." << std::endl;
	System.UIMutex.lock();
	System.ScheduleInitialize();

	/* Wait for process to finish touching UI and initializing */
	System.UIMutex.lock();
	System.UIMutex.unlock();
}

void ModbusFrame::Stop()
{
	/* Turn off all additional features; 
	 * Will be enabled in  Start() depending on 
	 * configuration */
	ui.MiddleSend->setEnabled(false);
	ui.SendAddress->setEnabled(false);
	ui.SendFunction->setEnabled(false);
	ui.LowSend->setEnabled(false);
	ui.TerminatedPing->setEnabled(false);

	/* Delete thread here! And block until it dies! */
	System.ScheduleShutdown();
}


void ModbusFrame::Status(const QString &Str, bool Error)
{
	QPalette qPalette;
	if (Error) {
		qPalette.setColor( QPalette::Foreground, QColor( Qt::red ) );
	} else {
		qPalette.setColor( QPalette::Foreground, QColor( Qt::black ) );
	}

	ui.Status->setPalette( qPalette );
	ui.Status->setText(Str);
}

void ModbusFrame::Finish()
{
	this->Stop();
	this->close();
}

void ModbusFrame::ConfigEnableUpdate()
{
	enum Proto {
		ModeASCII=0,
		ModeRTU,
		ModeTerminated,
	} Protocol = Proto(ui.MiddleProtocol->currentIndex());

	switch (Protocol)
	{
	case ModeTerminated:
		ui.TerminatedSelected->setEnabled(true);
		ui.TerminatedCustom->setEnabled(true);
		ui.TerminatedEcho->setEnabled(true);

		ui.ModbusMaster->setEnabled(false);
		ui.ModbusSlave->setEnabled(false);
		ui.ModbusTimeout->setEnabled(false);
		ui.ModbusRetries->setEnabled(false);
		ui.ModbusAddress->setEnabled(false);
		ui.ModbusMaster->setChecked(true);
		break;

	default:
		ui.TerminatedSelected->setEnabled(false);
		ui.TerminatedCustom->setEnabled(false);
		ui.TerminatedEcho->setEnabled(false);

		ui.ModbusMaster->setEnabled(true);
		ui.ModbusSlave->setEnabled(true);
		ui.ModbusTimeout->setEnabled(true);
		ui.ModbusRetries->setEnabled(true);
		ui.ModbusAddress->setEnabled(true);
		break;
	}
}

void ModbusFrame::MiddleSend()
{
	Mutex::Safe();
	try {
		System.CurrentProtocol->SendMessage(
			ParseEscapes(
				ui.SendData->text().toStdString()
				),
			ui.SendAddress->value(),
			ui.SendFunction->value());
	} catch (Error::Exception &e) {
		Status("Lowlevel error: " + tr(e.GetHeader())
		       + tr(e.GetDesc()), true);
		Stop();
	} catch (...) {
		Status(tr("Unknown error"), true);
	}
	Mutex::Unsafe();
}

void ModbusFrame::TerminatedPing()
{
	Mutex::Safe();
	Terminated *T = dynamic_cast<Terminated *>(this->System.CurrentProtocol);
	if (T) {
		T->Ping();
	}
	Mutex::Unsafe();
}

void ModbusFrame::LowSend()
{
	Mutex::Safe();
	try {
		System.CurrentLowlevel->SendString(
			ParseEscapes(
				ui.LowSendData->text().toStdString()
				)
			);
	} catch (Error::Exception &e) {
		Status(QString("Lowlevel error: ") + tr(e.GetHeader())
		       + tr(e.GetDesc()), true);
		Stop();
	} catch (...) {
		Status(tr("Unknown error"), true);
	}
	Mutex::Unsafe();
}

void ModbusFrame::UpdateError(int Errno, const char *Description)
{
	std::ostringstream ss;
	ss << tr(Error::StrError(Errno)).toStdString();
	if (Description) {
		ss << " (" << tr(Description).toStdString() << ")";
	}
	std::cerr << "Desc: = " << Description
		  << " Translated = "
		  << tr("Input high level data").toStdString();
	switch (Errno) {
	case Error::PING:
	case Error::PONG:
	case Error::INFO:
	case Error::OK:
		Status(ss.str().c_str());
		break;
	default:
		Status(ss.str().c_str(), true);
	}

	ss << std::endl;

	UpdateData(ss.str().c_str(), DataKind::ErrorOutput);

}

void ModbusFrame::UpdateData(const QString &Data, int DK)
{
	switch (DK) {
	case DataKind::MiddleInput:
		ui.MiddleInput->moveCursor(QTextCursor::End);
		ui.MiddleInput->insertPlainText(Data);
		break;

	case DataKind::MiddleOutput:
		ui.MiddleOutput->moveCursor(QTextCursor::End);
		ui.MiddleOutput->insertPlainText(Data);
		break;

	case DataKind::LowlevelInput:
		ui.LowlevelInput->moveCursor(QTextCursor::End);
		ui.LowlevelInput->insertPlainText(Data);
		break;

	case DataKind::LowlevelOutput:
		ui.LowlevelOutput->moveCursor(QTextCursor::End);
		ui.LowlevelOutput->insertPlainText(Data);
		break;

	case DataKind::ErrorOutput:
		ui.ErrorLog->moveCursor(QTextCursor::End);
		ui.ErrorLog->insertPlainText(Data);
		break;
	}
}

/******************************
 * Signal thread implementation
 *****************************/
Comm::Comm(QObject *parent, const Ui::ModbusFrame &ui)
	: QThread(parent), ui(ui)
{
	std::cerr << "Creating thread" << std::endl;

	DoAbort = DoInitialize = DoShutdown = false;

	/** Initialize all timers - must be called in GUI thread */
	for (int i=0; i<Timers; i++) {
		TimerUsed[i] = false;
		TimerConnected[i] = NULL;
		TimerPool[i].setSingleShot(true);
		connect(&TimerPool[i], SIGNAL(timeout()), this, SLOT(TimerTimeout()));
	}

	CurrentLowlevel = NULL;
	CurrentProtocol = NULL;
	CurrentTempProtocol = NULL;
	CurrentTerminated = false;

	connect(this, SIGNAL(_TimerStart(int, long)),
		this, SLOT(TimerStartSlot(int, long)));

	connect(this, SIGNAL(_TimerStop(int)),
		this, SLOT(TimerStopSlot(int)));




	start(LowPriority);
}

void Comm::ScheduleAbort()
{
	Mutex.lock();
	DoAbort = true;
	WaitCondition.wakeOne();
	Mutex.unlock();
}

void Comm::ScheduleInitialize()
{
	Mutex.lock();

	/** Deregister all timers by hand! */
	TimerInit();

	DoInitialize = true;
	WaitCondition.wakeOne();
	Mutex.unlock();
}

void Comm::ScheduleShutdown()
{
	Mutex.lock();

	/** Deregister all timers by hand! */
	TimerInit();

	DoShutdown = true;
	WaitCondition.wakeOne();
	Mutex.unlock();
}

void Comm::run()
{
	forever {
		std::cerr << "Thread Loop" << std::endl;

		Mutex.lock();
		if (DoAbort) {
			Mutex.unlock();
			return;
		}

		if (DoInitialize) {
			/* Lock UI! */
			DoInitialize = false;
			this->Initialize();
		} else if (DoShutdown) {
			DoShutdown = false;
			this->Shutdown();
		}
		/* Sleep waiting for GUI thread to schedule some action */
		WaitCondition.wait(&Mutex);
		Mutex.unlock();
	}
}

Comm::~Comm()
{
	std::cerr << "Destroying thread" << std::endl;

	ScheduleAbort();

	/* Wait for run() to finish */
	QThread::wait();
}

QString Comm::ToVisible(char Byte)
{
	if (Byte == '\\')
		return QString("\\\\");

	if (Byte >= ' ' && Byte <= '~')
		return QString(Byte);

	std::ostringstream ss;
	ss << std::hex << std::uppercase
	   << std::setfill('0') << "\\x" << std::setw(2)
	   << (unsigned int)((unsigned char)Byte);

	if (Byte == '\r' || Byte == '\n')
		ss << std::endl;

	return QString(ss.str().c_str());
}

bool Comm::Initialize()
{
	/** When entering this function
	 * our previous timers are already dead, so stop 
	 * whole communication system _now_ */

	/** Turn off previous comm system if any */
	Shutdown();

	std::cout << "Building comm system ";
	std::cerr << "in tid " << syscall(SYS_gettid) << std::endl;

	/* Gather some basic info */
	/* Serial */
	const std::string &SerialDevice = ui.SerialDevice->text().toStdString();
	/* Ugly! */
	const enum Config::BaudRate BaudRate =
		(enum Config::BaudRate) (13 - ui.SerialSpeed->currentIndex());

	enum Config::Parity Parity;
	switch (ui.SerialParity->currentIndex()) {
	case 0:	Parity = Config::EVEN; break;
	case 1:	Parity = Config::ODD; break;
	default: Parity = Config::NONE;	break;
	}

	enum Config::StopBits StopBits;
	if (ui.SerialStopbits->value() == 1) {
		StopBits = Config::SINGLE;
	} else {
		StopBits = Config::DOUBLE;
	}

	enum Config::CharSize CharSize = Config::CharSize8;
	switch (ui.SerialChar->value()) {
	case 5: CharSize = Config::CharSize5; break;
	case 6: CharSize = Config::CharSize6; break;
	case 7: CharSize = Config::CharSize7; break;
	default:
	case 8: CharSize = Config::CharSize8; break;
	}

	enum Config::FlowControl FlowControl = Config::FLOWNONE;
	switch (ui.SerialFlow->currentIndex()) {
	default:
	case 0: FlowControl = Config::FLOWNONE; break;
	case 1: FlowControl = Config::RTSCTS; break;
	case 2: FlowControl = Config::XONXOFF; break;
	}

	enum Proto {
		ModeASCII=0,
		ModeRTU,
		ModeTerminated,
	} Protocol = Proto(ui.MiddleProtocol->currentIndex());

	enum TermSelect {
		CR=0,
		LF,
		CRLF,
		Custom,
		None,
	} Terminator = TermSelect(Proto(ui.TerminatedSelected->currentIndex()));
	const std::string CustomTerminator = ui.TerminatedCustom->text().toStdString();

	std::string FinalTerminator;
	switch (Terminator) {
	case CR: FinalTerminator = "\r"; break;
	case LF: FinalTerminator = "\n"; break;
	case CRLF: FinalTerminator = "\r\n"; break;
	case Custom: FinalTerminator = CustomTerminator; break;
	case None: break;
	}

	const bool TerminatedEcho = ui.TerminatedEcho->isChecked();

	const int ReceiveTimeout = ui.MiddleTimeout->value();
	const int TransactionTimeout = ui.ModbusTimeout->value();

	const int Retries = ui.ModbusRetries->value();
	const bool MasterMode = ui.ModbusMaster->isChecked();

	/* Validate configuration */
	if (ui.ModbusSlave->isChecked()) {
		/* Check addresses of functions */
		const int A1 = ui.ModbusFunEchoNum->value();
		const int A2 = ui.ModbusFunTimeNum->value();
		const int A3 = ui.ModbusFunTextNum->value();
		const int A4 = ui.ModbusFunProgramNum->value();
		if (A1 == A2 || A1 == A3 || A1 == A4
		    || A2 == A3 || A2 == A4
		    || A3 == A4) {
			emit Status(tr("Slave function numbers must differ"), true);
			return false;
		}
	}

	if (Protocol == ModeTerminated && Terminator == Custom
	    && CustomTerminator.size() == 0) {
		UIMutex.unlock();
		emit Status(tr("Custom terminator can't be null"), true);
		return false;
	}

/*	if (CharSize == Config::CharSize7 && Protocol != ModeASCII) {
		UIMutex.unlock();
		emit Status(tr("Character size smaller than 8 bits allowed only with modbus ascii"), true);
		return false;
	} */

	/* Gather configuration variables and create comm system */
	if (ui.SerialSelected->isChecked()) {
		/* Create device */
		try {
			CurrentLowlevel = new Serial(BaudRate, Parity, StopBits,
						     CharSize, FlowControl,
						     SerialDevice.c_str());
		} catch (Error::Exception &e) {
			UIMutex.unlock();
			emit Status(tr("Serial error: ") +
				    tr(e.GetHeader()) + tr(e.GetDesc()), true);
			return false;
		} catch (...) {
			UIMutex.unlock();
			emit Status(tr("Unknown error while opening serial device"), true);
			return false;
		}
	} else {
		/* Network */
		const std::string Host = ui.NetworkHost->text().toStdString();
		const int Port = ui.NetworkPort->value();

		try {
			bool ServerMode = ui.NetworkMode->currentIndex() == 0 ? true : false;
#if 0
			if (ui.TCPSelected->isChecked()) {
				/* TCP */
				if (ServerMode) {
					/* Server mode */
					CurrentLowlevel = new NetworkTCPServer(Port);
				} else {
					/* Client mode */
					CurrentLowlevel = new NetworkTCPClient(Host.c_str(), Port);
				}
			} else
#endif
			{
				/* UDP */
				if (ServerMode) {
					/* Server mode */
					CurrentLowlevel = new NetworkUDPServer(Port);
				} else {
					/* Client mode */
					CurrentLowlevel = new NetworkUDPClient(Host.c_str(), Port);
				}
			}
		} catch (Error::Exception &e) {
			UIMutex.unlock();
			emit Status(tr("Network error: ") + tr(e.GetHeader())
				    + tr(e.GetDesc()), true);
			return false;
		} catch (...) {
			UIMutex.unlock();
			emit Status(tr("Error: Unable to open network connection"), true);
			return false;
		}
	}

	/** Lowlevel done - now create "lower middle level" layer */

	switch (Protocol) {
	case ModeASCII:
		CurrentTempProtocol = new ModbusASCII(this,
						      *CurrentLowlevel,
						      ReceiveTimeout);
		break;
	case ModeRTU:
		CurrentTempProtocol = new ModbusRTU(this,
						    *CurrentLowlevel,
						    ReceiveTimeout);
		break;
	case ModeTerminated:
		CurrentProtocol = new Terminated(this, *CurrentLowlevel, ReceiveTimeout,
						 FinalTerminator, TerminatedEcho);
		ui.TerminatedPing->setEnabled(true);
		CurrentTerminated = true;
		break;
	}

	if (Protocol == ModeASCII || Protocol == ModeRTU) {
		/* We have to set up master/slave protocols */
		if (MasterMode) {
			CurrentProtocol = new Master(this, *CurrentTempProtocol,
						     Retries, TransactionTimeout);
			ui.SendAddress->setEnabled(true);
			ui.SendFunction->setEnabled(true);
		} else {
			const int SlaveAddress = ui.ModbusAddress->value();
			Slave *S = new Slave(this, *CurrentTempProtocol,
						    SlaveAddress);

			if (ui.ModbusFunEcho->isChecked())
				S->EnableEcho(ui.ModbusFunEchoNum->value());
			if (ui.ModbusFunTime->isChecked())
				S->EnableTime(ui.ModbusFunTimeNum->value());
			if (ui.ModbusFunText->isChecked())
				S->EnableText(ui.ModbusFunTextNum->value(),
					      ui.ModbusFunTextArgs->text().toStdString());
			if (ui.ModbusFunProgram->isChecked())
				S->EnableExec(ui.ModbusFunProgramNum->value(),
					      ui.ModbusFunProgramArgs->text().toStdString());

			CurrentProtocol = S;
			ui.SendFunction->setEnabled(true);
		}
	}

	/* Enable interface buttons (disabled in Stop()) */
	ui.MiddleSend->setEnabled(true);
	ui.LowSend->setEnabled(true);

	UIMutex.unlock();

	if (Protocol == ModeTerminated)
		emit Status(tr("Terminated communication started"));
	else
		emit Status(tr("Modbus communication started"));
	return true;
}

void Comm::Shutdown()
{
	if (CurrentProtocol)
		delete CurrentProtocol, CurrentProtocol = NULL;
	if (CurrentTempProtocol)
		delete CurrentTempProtocol, CurrentTempProtocol = NULL;
	if (CurrentLowlevel)
		delete CurrentLowlevel, CurrentLowlevel = NULL;
	CurrentTerminated = false;
}


/***********************************
 * Protocol callbacks
 **********************************/

void Comm::ReceivedByte(char Byte)
{
	emit UpdateData(ToVisible(Byte), DataKind::LowlevelInput);
}

void Comm::SentByte(char Byte)
{
	emit UpdateData(ToVisible(Byte), DataKind::LowlevelOutput);
}

void Comm::ReceivedMessage(const std::string &Msg, int Address, int Function)
{
	std::ostringstream ss;
	if (!CurrentTerminated) {
		ss << "Addr="
		   << Address
		   << " Fun="
		   << Function
		   << " ";
	}
	ss << "Data='"
	   << Msg
	   << "'"
	   << std::endl;
	/* FIXME - ToVisible */
	emit UpdateData(ss.str().c_str(), DataKind::MiddleInput);

	/* Update status? */
	emit Status(tr("Recv: ") + ss.str().c_str());
}

void Comm::SentMessage(const std::string &Msg, int Address, int Function)
{
	std::ostringstream ss;
	if (!CurrentTerminated) {
		ss << "Addr="
		   << Address
		   << " Fun="
		   << Function
		   << " ";
	}

	ss << "Data='"
	   << Msg
	   << "'"
	   << std::endl;

	emit UpdateData(ss.str().c_str(), DataKind::MiddleOutput);
	emit Status(tr("Sent: ") + ss.str().c_str());
}

void Comm::Error(int Errno, const char *Description)
{
	emit UpdateError(Errno, Description);
	
	
}

/**************
 * Functions used inside the thread
 * to support timers 
 *************/

void Comm::TimerInit()
{
	for (int i=0; i<Timers; i++) {
		TimerPool[i].stop();
		TimerUsed[i] = false;
		TimerConnected[i] = NULL;
	}
}

/* Signal wrappers */
void Comm::TimerStart(int TimerID, long MSec)
{
	emit _TimerStart(TimerID, MSec);
}

void Comm::TimerStop(int TimerID)
{
	emit _TimerStop(TimerID);
}

int Comm::TimerRegister(Timeout *T)
{
	for (int i=0; i<Timers; i++) {
		if (!TimerUsed[i]) {
			TimerUsed[i] = true;
			TimerConnected[i] = T;
			return i;
		}
	}
	/* Ok, that's bad */
	std::cerr << "You've run out of timers! Increase pool or debug freeing them"
		  << std::endl;

	(void) *((char*)0);
	return NULL;
}

void Comm::TimerFree(int TimerID)
{
	TimerUsed[TimerID] = false;
	TimerConnected[TimerID] = NULL;
}

/* Slot catching timeouts() */
void Comm::TimerTimeout()
{
	QTimer *T = dynamic_cast<QTimer *>(sender());
	if (!T) {
		std::cerr << "Something is VERY bad. Check me."
			  << std::endl;
	}

	/* Find in table, and call good function */
	for (int i=0; i<Timers; i++) {
		if (&TimerPool[i] == T) {
			TimerConnected[i]->RunWrapper();
		}
	}
}

void Comm::TimerStartSlot(int TimerID, long MSec)
{
	std::cerr << "From main thread - starting timeout " 
		  << TimerID << " << - in "
		  << MSec << " miliseconds" << std::endl;
	TimerPool[TimerID].start(MSec);
}

void Comm::TimerStopSlot(int TimerID)
{
	std::cerr << "From main thread - stopping timeout " << TimerID << std::endl;

	TimerPool[TimerID].stop();
}


QMutex SafeMutex;
