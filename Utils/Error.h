#ifndef _ERROR_H_
#define _ERROR_H_

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
};



#endif
