#ifndef _MODBUS_FRAME_H_
#define _MODBUS_FRAME_H_

#include "Lowlevel/Lowlevel.h"
#include "Protocol/Protocol.h"

#include "Protocol/Modbus.h"
#include "Lowlevel/Serial.h"
#include "Lowlevel/Network.h"

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
		virtual void ReceivedMessage(int Address, int Function, const std::string &Msg);
		virtual void Error(int Errno);
		friend class ModbusFrame;
	};

	LowerCB LowerCB;

	/**@{ Elements of running communication system */
	Lowlevel *CurrentLowlevel;
	Protocol *CurrentProtocol;
	/*@} */

	/** Stop previous communication system */
	void Stop();

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

private:
	Ui::ModbusFrame ui;
};

#endif
