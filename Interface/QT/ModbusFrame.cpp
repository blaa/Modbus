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
	: QMainWindow(parent), LowerCB(*this)
{
	ui.setupUi(this);

	CurrentLowlevel = NULL;
	CurrentProtocol = NULL;
}

ModbusFrame::~ModbusFrame()
{
	Stop();
}

void ModbusFrame::Stop()
{

	ui.MiddleSend->setEnabled(false);
	ui.LowSend->setEnabled(false);
	ui.MiddlePing->setEnabled(false);

	if (CurrentProtocol)
		delete CurrentProtocol,	CurrentProtocol = NULL;
	if (CurrentLowlevel)
		delete CurrentLowlevel, CurrentLowlevel = NULL;
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

void ModbusFrame::Finish()
{
	this->Stop();
	this->close();
}

void ModbusFrame::Start()
{
	std::cerr << "Start pressed" << std::endl;
	Stop();

	/* Gather configuration variables and create comm system */

	/* TODO: How will it work with retranslate? */
	if (ui.SerialSelected->isChecked()) {
		/* Serial */
		const char *Device = ui.SerialDevice->text().toStdString().c_str();
		/* Ugly! */
		const enum Config::BaudRate BaudRate =
			(enum Config::BaudRate) (13 - ui.SerialSpeed->currentIndex());

		std::cerr << "baudrate = " << BaudRate << std::endl;

		enum Config::Parity Parity;
		if (ui.SerialParity->currentText() == "Even") {
			Parity = Config::EVEN;
		} else if (ui.SerialParity->currentText() == "Odd") {
			Parity = Config::ODD;
		} else {
			Parity = Config::NONE;
		}
		
		enum Config::StopBits StopBits;
		if (ui.SerialStopbits->currentText() == "1") {
			StopBits = Config::SINGLE;
		} else {
			StopBits = Config::DOUBLE;
		}

		/* Create device */
		try {

			CurrentLowlevel = new Serial(BaudRate, Parity, StopBits,
						     Config::CharSize8, Device);
		} catch (Error::Exception &e) {
			ui.Status->setText(QString("Serial error: ") + e.what());
			return;
		} catch (...) {
			ui.Status->setText("Unknown error while opening serial device");
			return;
		}

	} else {
		
		/* Network */
		const std::string Host = ui.NetworkHost->text().toStdString();
		const int Port = ui.NetworkPort->value();

		try {
			if (ui.TCPSelected->isChecked()) {
				if (this->ui.NetworkMode->currentText() == "Server") {
					/* Server mode */
					CurrentLowlevel = new NetworkTCPServer(Port);
				} else {
					/* Client mode */
					CurrentLowlevel = new NetworkTCPClient(Host.c_str(), Port);
				}
			} else {
				/* UDP */
				if (this->ui.NetworkMode->currentText() == "Server") {
					/* Server mode */
					CurrentLowlevel = new NetworkUDPServer(Port);
				} else {
					/* Client mode */
					CurrentLowlevel = new NetworkUDPClient(Host.c_str(), Port);
				}
			}
		} catch (Error::Exception &e) {
			ui.Status->setText(QString("Network error: ") + e.what());
			return;
		} catch (...) {
			std::cerr << "Network connect failed!\n" << std::endl;
			ui.Status->setText("Error: Unable to open network connection");
			return;
		}
	}

	const int Timeout = ui.MiddleTimeout->value();
	const int Address = ui.MiddleAddress->value();

	if (ui.MiddleProtocol->currentText() == "Modbus ASCII") {
		CurrentProtocol = new ModbusASCII(&LowerCB, *CurrentLowlevel, Timeout);
		ui.Status->setText("Modbus ASCII communication started");
	} else {
		CurrentProtocol = new ModbusRTU(&LowerCB, *CurrentLowlevel, Timeout);
		ui.Status->setText("Modbus RTU communication started");
	}
	std::cerr << "Creation done" << std::endl;

	/* Enable interface buttons (disabled in Stop()) */
	ui.MiddleSend->setEnabled(true);
	ui.LowSend->setEnabled(true);
	ui.MiddlePing->setEnabled(true);
}

void ModbusFrame::MiddleSend()
{
	CurrentProtocol->SendMessage(
		ParseEscapes(
			ui.SendData->text().toStdString()
			),
		ui.SendAddress->value(),
		ui.SendFunction->value());
}

void ModbusFrame::LowSend()
{
	CurrentLowlevel->SendString(
		ParseEscapes(
			ui.LowSendData->text().toStdString()
			)
		);
}

void ModbusFrame::MiddlePing()
{
}


/******************************
 * Callback implementation 
 *****************************/
ModbusFrame::LowerCB::LowerCB(ModbusFrame &MF) : MF(MF)
{
}

void ModbusFrame::LowerCB::ReceivedByte(char Byte)
{
	MF.ui.LowlevelInput->insertPlainText(QString(Byte));
}

void ModbusFrame::LowerCB::SentByte(char Byte)
{
	MF.ui.LowlevelOutput->insertPlainText(QString(Byte));
}

void ModbusFrame::LowerCB::ReceivedMessage(int Address, int Function, const std::string &Msg)
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
	MF.ui.MiddleInput->insertPlainText(ss.str().c_str());

	MF.ui.Status->setText(("Recv: " + ss.str()).c_str());
}

void ModbusFrame::LowerCB::SentMessage(int Address, int Function, const std::string &Msg)
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
	MF.ui.MiddleOutput->insertPlainText(ss.str().c_str());

	MF.ui.Status->setText(("Sent: " + ss.str()).c_str());
}


void ModbusFrame::LowerCB::Error(int Errno, const char *Description)
{
	std::ostringstream ss;
	ss << Error::StrError(Errno);
	if (Description) {
		ss << " (" << Description << ")";
	}

	MF.ui.Status->setText(ss.str().c_str());

	ss << std::endl;

	MF.ui.ErrorLog->insertPlainText(ss.str().c_str());
}


