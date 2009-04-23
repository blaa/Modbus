#ifndef _LOWLEVEL_H_
#define _LOWLEVEL_H_

#include <string>

/** Interface of all low-level communication methods 
 * - most obviously the interface of a serial.
 *
 * Implementation must be interrupt/signal based. Because of this
 * we will have to inform higher levels of incoming data.
 * As a callback we will define an abstract class which will be 
 * implemented by higher layer and passed to the initializer of our class.
 *
 */
class Lowlevel {
public:

	/** Callback interface declaration */
	class Callback
	{
	public:
		/** Called when we receive a single byte. */
		virtual void ByteReceived(char Byte) = 0;
		/** Called on any error; to be defined */
		virtual void Error(int Errno) = 0;
	};
	
	/** Registers middle-level callback */
	virtual void RegisterCallback(Callback *CB) = 0;

	/*** Low-level interface ***/
	/** Function initializes transmission of a single byte.
	 * It might block until the send buffer is empty */
	virtual void SendByte(char Byte) = 0;

	/** If there's a byte waiting in a queue - return it, 
	 * otherwise returns -1. */
	virtual int GetByte() = 0;

	/*** High-level interface ***/
	/** Non-Blocking function which initializes transmission of 
	 * the data stored in passed buffer. Buffer must continue
	 * to exist until all data is sent.
	 *
	 * Function might block only if the previous buffer is still
	 * being send, not for the whole transmission period.
	 *
	 * We can use std::vector instead of std::string; or 
	 * even char* + size or our own class, as you wish. */
	virtual void SendString(const std::string &Buffer);
};

#endif
