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
#ifndef _ERROR_H_
#define _ERROR_H_

#include <string>
#include <exception>

namespace Error {
	enum Type {
		OK = 0,		/* No error */
		INFO,		/* Show user information */
		WARNING,	/* Show user warning */
		ERROR,		/* General error */

		HASH,		/* Hash error = CRC/LRC */

		/* Frame error - e.g. not beginning with ':' 
		 * Or not ending with CR LF in MODBUS ASCII */
		FRAME,

		/* Timeout during frame receive (got some bytes, then silence) */
		TIMEOUT,
		
		/* Received message for wrong address - ignoring */
		ADDRESS,

		/* Not really an error - we've got a PING, replied with PONG */
		PING,

		/* Pong received */
		PONG,
	};

	/** Convert error code into description string */
	const char *StrError(int Errno);

	/** Class for informing about problems with the use of exceptions */
	class Exception : public std::exception
	{
		/**@{ Error header and description -
		 * they are splitted so we can more easily translate them */
		std::string Header, Description;
		/*@}*/
	public:
		/** Create exception with description only */
		Exception(const std::string &Description) throw()
		{
			this->Description = Description;
		}

		/** Create exception with description and header */
		Exception(const std::string &Header,
			  const std::string &Description) throw()
		{
			this->Header = Header;
			this->Description = Description;
		}

		~Exception() throw()
		{
		}

		/** Return combined description of error */
		const char *what() const throw()
		{
			if (Header.size() > 0)
				return (Header + Description).c_str();
			return Description.c_str();
		}

		/** Return a header of error */
		const char *GetHeader() const throw()
		{
			return Header.c_str();
		}

		/** Return a description of error */
		const char *GetDesc() const throw()
		{
			return Description.c_str();
		}
	};
};



#endif
