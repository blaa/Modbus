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

#include <QtCore/QString>
#include <QtGui/QMessageBox>

#include "ModbusFrame.h"
#include "Utils/Error.h"
#include "Lowlevel/Safe.h"

ModbusFrame::ModbusFrame(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	CurrentLowlevel = NULL;
	CurrentProtocol = NULL;
	CurrentTempProtocol = NULL;
}

ModbusFrame::~ModbusFrame()
{
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

	if (CurrentProtocol)
		delete CurrentProtocol, CurrentProtocol = NULL;
	if (CurrentTempProtocol)
		delete CurrentTempProtocol, CurrentTempProtocol = NULL;
	if (CurrentLowlevel)
		delete CurrentLowlevel, CurrentLowlevel = NULL;

	CurrentTerminated = false;
}


void ModbusFrame::StatusInfo(const QString &Str)
{
	QPalette qPalette;

	qPalette.setColor( QPalette::Foreground, QColor( Qt::black ) );
	ui.Status->setPalette( qPalette );

	ui.Status->setText(Str);
}

void ModbusFrame::StatusError(const QString &Str)
{
	QPalette qPalette;

	qPalette.setColor( QPalette::Foreground, QColor( Qt::red ) );
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

void ModbusFrame::Start()
{
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
			StatusError(tr("Slave function numbers must differ"));
			return;
		}
	}

	if (Protocol == ModeTerminated && Terminator == Custom 
	    && CustomTerminator.size() == 0) {
		StatusError(tr("Custom terminator can't be null"));
		return;
	}

	if (CharSize == Config::CharSize7 && Protocol != ModeASCII) {
		/* FIXME: Shall we keep it? */
		StatusError(tr("Character size smaller than 8 bits allowed only with modbus ascii"));
		return;
	}

	Stop();

	/* Gather configuration variables and create comm system */
	if (ui.SerialSelected->isChecked()) {

		/* Create device */
		try {
			CurrentLowlevel = new Serial(BaudRate, Parity, StopBits,
						     CharSize, FlowControl,
						     SerialDevice.c_str());
		} catch (Error::Exception &e) {
			StatusError(tr("Serial error: ") + tr(e.GetHeader()) + tr(e.GetDesc()));
			return;
		} catch (...) {
			StatusError(tr("Unknown error while opening serial device"));
			return;
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
			StatusError(tr("Network error: ") + tr(e.GetHeader())
					   + tr(e.GetDesc()));
			return;
		} catch (...) {
			StatusError(tr("Error: Unable to open network connection"));
			return;
		}
	}

	/** Read data about middle protocol configuration */

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

			CurrentProtocol = S;
			ui.SendFunction->setEnabled(true);
		}
	}

	/* Enable interface buttons (disabled in Stop()) */
	ui.MiddleSend->setEnabled(true);
	ui.LowSend->setEnabled(true);

	if (Protocol == ModeTerminated)
		StatusInfo(tr("Terminated communication started"));
	else
		StatusInfo(tr("Modbus communication started"));
}

void ModbusFrame::MiddleSend()
{
	Mutex::Safe();
	try {
		CurrentProtocol->SendMessage(
			ParseEscapes(
				ui.SendData->text().toStdString()
				),
			ui.SendAddress->value(),
			ui.SendFunction->value());
	} catch (Error::Exception &e) {
		StatusError("Lowlevel error: " + tr(e.GetHeader())
			    + tr(e.GetDesc()));
		Stop();
	} catch (...) {
		StatusError(tr("Unknown error"));
	}
	Mutex::Unsafe();
}

void ModbusFrame::TerminatedPing()
{
	Terminated *T = dynamic_cast<Terminated *>(this->CurrentProtocol);
	if (T) {
		Mutex::Safe();
		T->Ping();
		Mutex::Unsafe();
	}
}

void ModbusFrame::LowSend()
{
	Mutex::Safe();
	try {
		CurrentLowlevel->SendString(
			ParseEscapes(
				ui.LowSendData->text().toStdString()
				)
			);
	} catch (Error::Exception &e) {
		StatusError(QString("Lowlevel error: ") + tr(e.GetHeader())
				   + tr(e.GetDesc()));
		Stop();
	} catch (...) {
		StatusError(tr("Unknown error"));
	}
	Mutex::Unsafe();
}


/******************************
 * Callback implementation 
 *****************************/
QString ModbusFrame::ToVisible(char Byte)
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
     

void ModbusFrame::ReceivedByte(char Byte)
{
	Mutex::Safe();
	ui.LowlevelInput->moveCursor(QTextCursor::End);
	ui.LowlevelInput->insertPlainText(ToVisible(Byte));
	Mutex::Unsafe();
}

void ModbusFrame::SentByte(char Byte)
{
	Mutex::Safe();
	ui.LowlevelOutput->moveCursor(QTextCursor::End);
	ui.LowlevelOutput->insertPlainText(ToVisible(Byte));
	Mutex::Unsafe();
}


void ModbusFrame::ReceivedMessage(const std::string &Msg, int Address, int Function)
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
	Mutex::Safe();
	ui.MiddleInput->moveCursor(QTextCursor::End);
	ui.MiddleInput->insertPlainText(ss.str().c_str());
	Mutex::Unsafe();


//	ui.Status->setText(("Recv: " + ss.str()).c_str());
}

void ModbusFrame::SentMessage(const std::string &Msg, int Address, int Function)
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
	Mutex::Safe();
	ui.MiddleOutput->moveCursor(QTextCursor::End);
	ui.MiddleOutput->insertPlainText(ss.str().c_str());
	Mutex::Unsafe();
//	ui.Status->setText(("Sent: " + ss.str()).c_str());
}


void ModbusFrame::Error(int Errno, const char *Description)
{
	std::ostringstream ss;
	ss << tr(Error::StrError(Errno)).toStdString();
	if (Description) {
		ss << " (" << tr(Description).toStdString() << ")";
	}
	switch (Errno) {
	case Error::PING:
	case Error::PONG:
	case Error::INFO:
	case Error::OK:
		StatusInfo(ss.str().c_str());
		break;
	default:
		StatusError(ss.str().c_str());
	}

//	ui.Status->setText(ss.str().c_str());
	ss << std::endl;
	Mutex::Safe();
	ui.ErrorLog->moveCursor(QTextCursor::End);
	ui.ErrorLog->insertPlainText(ss.str().c_str());
	Mutex::Unsafe();
}


/** Update interface! */
void ModbusFrame::Run()
{
	
}
