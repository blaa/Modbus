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
#ifndef _MODBUS_FRAME_H_
#define _MODBUS_FRAME_H_

#include <string>

#include "Lowlevel/Lowlevel.h"
#include "Protocol/Protocol.h"

#include "Protocol/Modbus.h"
#include "Lowlevel/Serial.h"
#include "Lowlevel/NetworkTCP.h"
#include "Lowlevel/NetworkUDP.h"


#include "ui_ModbusFrame.h"

class ModbusFrame : public QMainWindow
{
	Q_OBJECT

	/** Protocol callback implementation */
	class LowerCB : public Protocol::Callback
	{
		ModbusFrame &MF;
		LowerCB(ModbusFrame &MF);
	public:
		virtual void ReceivedByte(char Byte);
		virtual void SentByte(char Byte);
		virtual void ReceivedMessage(int Address, int Function, const std::string &Msg);
		virtual void SentMessage(int Address, int Function, const std::string &Msg);
		virtual void Error(int Errno, const char *Description);
		friend class ModbusFrame;
	};

	LowerCB LowerCB;

	/**@{ Elements of running communication system */
	Lowlevel *CurrentLowlevel;
	Protocol *CurrentProtocol;
	/*@} */

	/** Stop previous communication system */
	void Stop();

	/** Convert \xXX into real characters and \\ into \ */
	const std::string ParseEscapes(const std::string &Str);

public:
	ModbusFrame(QWidget *parent = NULL);
	~ModbusFrame();

private slots:
	/** Recreate all objects - initialize interfaces and communication */
	void Start();
	/** Send a modbus frame using data from GUI */
	void MiddleSend();
	/** Send a string using lowlevel interface */
	void LowSend();
	/** Send a ping using modbus (predefined function) */
	void MiddlePing();
	/** Cleanup and close the window */
	void Finish();
private:
	Ui::ModbusFrame ui;
};

#endif
