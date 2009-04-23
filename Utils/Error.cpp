#include "Error.h"

const char *Error::StrError(int Errno)
{
	switch (Errno) {
	case OK: return "No error";
	case HASH: return "Hash (CRC/LRC) error";
	case FRAME: return "Frame error";
	case TIMEOUT: return "Timeout error";
	default: return "Unknown error";
	}
}


