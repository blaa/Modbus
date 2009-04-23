#include "Error.h"

const char *Error::StrError(int Errno)
{
	switch (Errno) {
	case OK: return "No error";
	default: return "Unknown error";
	}
}


