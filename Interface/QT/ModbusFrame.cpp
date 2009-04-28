#include <iostream>
#include <string>
#include <sstream>

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
	if (CurrentProtocol)
		delete CurrentProtocol,	CurrentProtocol = NULL;
	if (CurrentLowlevel)
		delete CurrentLowlevel, CurrentLowlevel = NULL;
}

void ModbusFrame::Start()
{
	std::cerr << "Start pressed" << std::endl;
	Stop();

	/* Gather configuration variables and create comm system */

	/* TODO: How will it work with retranslate? */

	if (ui.NetworkSelected->isEnabled()) {
		/* Network */
		const std::string Host = ui.NetworkHost->text().toStdString();
		const int Port = ui.NetworkPort->value();

		try {
			if (this->ui.NetworkMode->currentText() == "Server") {
				/* Server mode */
				CurrentLowlevel = new NetworkTCPServer(Port);
			} else {
				/* Client mode */
				CurrentLowlevel = new NetworkTCPClient(Host.c_str(), Port);
			}
		} catch (...) {
			std::cerr << "Network connect failed!\n" << std::endl;
			QMessageBox::information(this, "Error", "Unable to open network connection");
			return;
		}

	} else {
		/* Serial */
		const char *Device = ui.SerialDevice->text().toStdString().c_str();
		/* Ugly! */
		const enum Config::BaudRate BaudRate =
			(enum Config::BaudRate) (13 - ui.SerialSpeed->currentIndex());

		enum Config::Parity Parity;
		if (ui.SerialParity->currentText() == "Even") {
			Parity = Config::EVEN;
		} else {
			Parity = Config::ODD;
		}
		
		enum Config::StopBits StopBits;
		if (ui.SerialStopbits->currentText() == "1") {
			StopBits = Config::SINGLE;
		} else {
			StopBits = Config::DOUBLE;
		}

		/* Create device */
		CurrentLowlevel = new Serial(BaudRate, Parity, StopBits,
					     Config::CharSize8, Device);

	}

	const int Timeout = ui.MiddleTimeout->value();
	const int Address = ui.MiddleAddress->value();


	if (ui.MiddleProtocol->currentText() == "Modbus ASCII") {
		CurrentProtocol = new ModbusASCII(&LowerCB, *CurrentLowlevel, Timeout);
	} else {
		CurrentProtocol = new ModbusRTU(&LowerCB, *CurrentLowlevel, Timeout);
	}
	std::cerr << "Creation done" << std::endl;
}

void ModbusFrame::MiddleSend()
{
	CurrentProtocol->SendMessage(
		ui.SendData->text().toStdString(),
		ui.SendAddress->value(),
		ui.SendFunction->value());
}

void ModbusFrame::LowSend()
{
	CurrentLowlevel->SendString(
		ui.LowSendData->text().toStdString());
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
}


void ModbusFrame::LowerCB::Error(int Errno, const char *Description)
{
	if (Description) {
		MF.ui.ErrorLog->insertPlainText(
			QString(Error::StrError(Errno)) + "(" + Description + ")\n");
	}
	else {
		MF.ui.ErrorLog->insertPlainText(
			QString(Error::StrError(Errno)) + "\n");

	}
}


