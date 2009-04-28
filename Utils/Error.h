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
		HASH,		/* Hash error = CRC/LRC */

		/* Frame error - e.g. not beginning with ':' 
		 * Or not ending with CR LF in MODBUS ASCII */
		FRAME,

		/* Timeout during frame receive (got some bytes, then silence) */
		TIMEOUT,
	};

	const char *StrError(int Errno);

	/** Class for informing about problems with the use of exceptions */
	class Exception : public std::exception
	{
		std::string Header, Description;
	public:
		
		Exception(const std::string &Description) throw()
		{
			this->Description = Description;
		}

		Exception(const std::string &Header,
			  const std::string &Description) throw()
		{
			this->Header = Header;
			this->Description = Description;
		}

		~Exception() throw()
		{
		}

		const char *what() const throw()
		{
			if (Header.size() > 0)
				return (Header + Description).c_str();
			return Description.c_str();
		}
	};
};



#endif
