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

#include "Lowlevel/NetworkTCP.h"
#include "Lowlevel/NetworkUDP.h"
#include "Lowlevel/Serial.h"

#include "Protocol/MasterSlave.h"
#include "Protocol/Modbus.h"
#include "Protocol/Terminated.h"


#include "ui_ModbusFrame.h"


/** Class implementing actions of main window GUI */
class ModbusFrame : public QMainWindow, public Protocol::Callback
{
	Q_OBJECT

	/**@{ Elements of running communication system */
	Lowlevel *CurrentLowlevel;
	/** Protocol with which we talk. Master/slave or terminated */
	Protocol *CurrentProtocol;
	/** Hidden underlying protocol: modbus ascii, 
	 *  modbus rtu or null (if terminated enabled) */
	Protocol *CurrentTempProtocol;
	/*@} */

	/** Stop previous communication system */
	void Stop();

	/** Convert \xXX \n \r into real characters and \\ into \ */
	const std::string ParseEscapes(const std::string &Str);

	/** Display red status information */
	void StatusError(const QString &Str);
	
	/** Display normal status information */
	void StatusInfo(const QString &Str);

public:
	/** Create GUI */
	ModbusFrame(QWidget *parent = NULL);
	~ModbusFrame();

	/**@{ Protocol callback interface */
	virtual void ReceivedByte(char Byte);
	virtual void SentByte(char Byte);
	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function);
	virtual void SentMessage(const std::string &Msg, int Address, int Function);
	virtual void Error(int Errno, const char *Description);
	/* @} */

private slots:
	/** Recreate all objects - initialize interfaces and communication */
	void Start();
	/** Send a modbus frame using data from GUI */
	void MiddleSend();
	/** Send a string using lowlevel interface */
	void LowSend();
	/** Cleanup and close the window */
	void Finish();

private:
	/** Main window definition created with designer */
	Ui::ModbusFrame ui;
};

#endif
