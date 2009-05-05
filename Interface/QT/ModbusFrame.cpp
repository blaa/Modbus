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
#include <ctype.h>

#include <QtCore/QString>
#include <QtGui/QMessageBox>

#include "ModbusFrame.h"
#include "Utils/Error.h"

ModbusFrame::ModbusFrame(QWidget *parent)
	: QMainWindow(parent)//, LowerCB(*this)
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
				New += '\n';
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
	ui.MiddleSend->setEnabled(false);
	ui.LowSend->setEnabled(false);
//	ui.MiddlePing->setEnabled(false);

	if (CurrentProtocol)
		delete CurrentProtocol, CurrentProtocol = NULL;
	if (CurrentTempProtocol)
		delete CurrentTempProtocol, CurrentTempProtocol = NULL;
	if (CurrentLowlevel)
		delete CurrentLowlevel, CurrentLowlevel = NULL;
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

void ModbusFrame::Start()
{
	Stop();

	/* Gather configuration variables and create comm system */
	if (ui.SerialSelected->isChecked()) {
		/* Serial */
		const char *Device = ui.SerialDevice->text().toStdString().c_str();
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

		/* Create device */
		try {
			CurrentLowlevel = new Serial(BaudRate, Parity, StopBits,
						     Config::CharSize8, FlowControl,
						     Device);
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
			if (ui.TCPSelected->isChecked()) {
				/* TCP */
				if (ServerMode) {
					/* Server mode */
					CurrentLowlevel = new NetworkTCPServer(Port);
				} else {
					/* Client mode */
					CurrentLowlevel = new NetworkTCPClient(Host.c_str(), Port);
				}
			} else {
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
			StatusError("Network error: " + tr(e.GetHeader())
					   + tr(e.GetDesc()));
			return;
		} catch (...) {
			StatusError("Error: Unable to open network connection");
			return;
		}
	}

	/** Read data about middle protocol configuration */
	const int ReceiveTimeout = ui.MiddleTimeout->value();


	enum Proto {
		ModeASCII=0,
		ModeRTU,
		ModeTerminated,
	} Protocol = Proto(ui.MiddleProtocol->currentIndex());

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
//		CurrentProtocol = new Terminated(&LowerCB, *CurrentLowlevel, ReceiveTimeout);
		break;
	}


	const bool MasterMode = ui.ModbusMaster->isChecked();

	if (Protocol == ModeASCII || Protocol == ModeRTU) {
		/* We have to set up master/slave protocols */
		if (MasterMode) {
			const int TransactionTimeout = ui.ModbusTimeout->value();
			const int Retries = ui.ModbusRetries->value();
			CurrentProtocol = new Master(this, *CurrentTempProtocol, 
						     Retries, TransactionTimeout);
		} else {
			const int SlaveAddress = ui.ModbusAddress->value();
			Slave *S = new Slave(this, *CurrentTempProtocol, 
						    SlaveAddress);

			CurrentProtocol = S;
		}
//		ui.MiddlePing->setEnabled(true);
	}

	/* Enable interface buttons (disabled in Stop()) */
	ui.MiddleSend->setEnabled(true);
	ui.LowSend->setEnabled(true);
	StatusInfo(tr("Modbus communication started"));
}

void ModbusFrame::MiddleSend()
{
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
}

void ModbusFrame::LowSend()
{
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
}


/******************************
 * Callback implementation 
 *****************************/
void ModbusFrame::ReceivedByte(char Byte)
{
	ui.LowlevelInput->insertPlainText(QString(Byte));
}

void ModbusFrame::SentByte(char Byte)
{
	ui.LowlevelOutput->insertPlainText(QString(Byte));
}


void ModbusFrame::ReceivedMessage(const std::string &Msg, int Address, int Function)
{
	std::ostringstream ss;
	ss << "Addr=" 
	   << Address
	   << " Fun="
	   << Function
	   << " Data='"
	   << Msg
	   << "'"
	   << std::endl;
	ui.MiddleInput->insertPlainText(ss.str().c_str());
	ui.Status->setText(("Recv: " + ss.str()).c_str());
}

void ModbusFrame::SentMessage(const std::string &Msg, int Address, int Function)
{
	std::ostringstream ss;
	ss << "Addr="
	   << Address
	   << " Fun="
	   << Function
	   << " Data='"
	   << Msg
	   << "'"
	   << std::endl;
	ui.MiddleOutput->insertPlainText(ss.str().c_str());

	ui.Status->setText(("Sent: " + ss.str()).c_str());
}


void ModbusFrame::Error(int Errno, const char *Description)
{
	std::ostringstream ss;
	ss << Error::StrError(Errno);
	if (Description) {
		ss << " (" << Description << ")";
	}

	ui.Status->setText(ss.str().c_str());

	ss << std::endl;

	ui.ErrorLog->insertPlainText(ss.str().c_str());
}


