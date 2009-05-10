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

#include <QtCore/QSemaphore>
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>
#include <QtCore/QMutex>

#include "Lowlevel/Lowlevel.h"
#include "Protocol/Protocol.h"

#include "Lowlevel/NetworkTCP.h"
#include "Lowlevel/NetworkUDP.h"
#include "Lowlevel/Serial.h"

#include "Utils/Timeout.h"
#include "Protocol/Terminated.h"
#include "Protocol/Modbus.h"
#include "Protocol/MasterSlave.h"




#include "ui_ModbusFrame.h"


namespace DataKind {
	enum DataKind {
		MiddleInput=0, MiddleOutput,
		LowlevelInput, LowlevelOutput,
		ErrorOutput
	};
};

extern QMutex SafeMutex;

/** Thread running communication system allowing reentrant-safe
 * reception of signals
 */
class Comm : public QThread, public Protocol::Callback
{
	Q_OBJECT

	/**@{ Elements of running communication system */
	Lowlevel *CurrentLowlevel;
	/** Protocol with which we talk. Master/slave or terminated */
	Protocol *CurrentProtocol;
	/** Hidden underlying protocol: modbus ascii,
	 *  modbus rtu or null (if terminated enabled) */
	Protocol *CurrentTempProtocol;

	/** Used to check whether to print Fun and Addr */
	bool CurrentTerminated;
	/*@} */

	/**@{ Helpers */
	/** Convert unprintable characters to \xXX notation */
	QString ToVisible(char Byte);
	/*@}*/

	/** Initialize communication system using data from UI */
	bool Initialize();

	/** Shutdown communication system */
	void Shutdown();

	/**@{Thread safe functions and variables to tell run() what to do next */
	/** For sleeping thread if nothing to do */
	QWaitCondition WaitCondition;
	/** Secures access to all common variables */
	QMutex Mutex;
	/** Finish thread if true */
	bool DoAbort;
	/** Start communication system if true */
	bool DoInitialize;
	/** Stop the communication system if true */
	bool DoShutdown;

	void ScheduleInitialize();
	void ScheduleShutdown();
	void ScheduleAbort();
	/**@}*/


	/** Interface from which we gather information
	 * about configuration; access restricted with mutex */
	const Ui::ModbusFrame &ui;

	/** Secure access to ui */
	QMutex UIMutex;

	/**@{Timer pool */
	static const int Timers = 10;
	QTimer TimerPool[Timers];
	Timeout *TimerConnected[Timers];
	bool TimerUsed[Timers];

	/** Initialize timers - must be called by GUI thread */
	void TimerInit();
	/*@}*/

protected:
	/** Thread main-loop implementation */
	void run();

public:
	Comm(QObject *parent, const Ui::ModbusFrame &ui);
	~Comm();

	/**@{ Protocol callback interface */
	virtual void ReceivedByte(char Byte);
	virtual void SentByte(char Byte);
	virtual void ReceivedMessage(const std::string &Msg, int Address, int Function);
	virtual void SentMessage(const std::string &Msg, int Address, int Function);
	virtual void Error(int Errno, const char *Description);
	/* @} */

	/** Wrapper emitting _TimerStart signal */
	void TimerStart(int TimerID, long MSec);
	/** Wrapper emitting _TimerStop signal */
	void TimerStop(int TimerID);

	/** Connects timeout with timer and returns timer ID */
	int TimerRegister(Timeout *Timeout);

	/** Frees TimerID for future use */
	void TimerFree(int TimerID);

	friend class ModbusFrame;
signals:
	/** Update data in main window */
	void UpdateData(const QString &Data, int DK);

	/** Update error in main window */
	void UpdateError(int Errno, const char *Description);

	/** Set status in main window */
	void Status(const QString &Str, bool Error = false);

	/** Pass timer to GUI thread which starts it. */
	void _TimerStart(int TimerID, long MSec);

	/** Stop timer with specified ID */
	void _TimerStop(int TimerID);

public slots:
	/** Slot called when any of scheduled timers timeouts */
	void TimerTimeout();

	/** Function which starts given timeout */
	void TimerStartSlot(int TimerID, long MSec);

	/** Function stopping given timeout */
	void TimerStopSlot(int TimerID);
};


/** Class implementing actions of main window GUI */
class ModbusFrame : public QMainWindow
{
	Q_OBJECT

	/** Stop previous communication system */
	void Stop();

	/** Convert \xXX \n \r into real characters and \\ into \ */
	const std::string ParseEscapes(const std::string &Str);

	/** Current communication system */
	Comm System;

public:
	/** Create GUI */
	ModbusFrame(QWidget *parent = NULL);
	~ModbusFrame();

private slots:
	/** Recreate all objects - initialize interfaces and communication */
	void Start();
	/** Send a modbus frame using data from GUI */
	void MiddleSend();
	/** Send a string using lowlevel interface */
	void LowSend();
	/** Ping second station using terminated protocol */
	void TerminatedPing();
	/** Cleanup and close the window */
	void Finish();
	/** Enable/disable some configuration fields */
	void ConfigEnableUpdate();

public slots:
	/** Display status information */
	void Status(const QString &Str, bool Error = false);

	/** Update display */
	void UpdateData(const QString &Data, int DK);

	/** Update error display */
	void UpdateError(int Errno, const char *Description);

private:
	/** Main window definition created with designer */
	Ui::ModbusFrame ui;
};



#endif
