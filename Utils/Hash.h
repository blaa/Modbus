#ifndef _CRC_H_
#define _CRC_H_

#include <inttypes.h>

#define CRC_LO8(x) ((unsigned char)(x&0x00FF))
#define CRC_HI8(x) (CRC_LO8(x>>8))

/** General hash interface used to create CRC16 and LRC */
template<typename T, T InitValue = T()>
class Hash {
public:
	/** Type used for hash */
	typedef T Hash_t;

	/** Hash function internal state */
	Hash_t State;

	/** Initialize hash function */
	inline void Init()
	{
		State = InitValue;
	}

	/** Initialize hash algorithm */
	Hash()
	{
		this->Init();
	}

	/** Return final value */
	T Get()
	{
		return State;
	}

	/** Ask if calculated hash is correct (equal zero) */
	bool IsCorrect()
	{
		if (State == Hash_t())
			return true;
		return false;
	}
};


/** CRC16 implementation */
class CRC16 : public Hash<uint16_t, 0xFFFF> {
public:
	/** Update CRC state with a single byte */
	inline void Update(unsigned char data)
	{
		State ^= data;
		for (int i = 0; i < 8; ++i)
		{
			if (State & 1)
				State = (State >> 1) ^ 0xA001;
			else
				State = (State >> 1);
		}
	}
};

/** LRC implementation for MODBUS ASCII */
class LRC : public Hash<uint8_t, 0x00> {
public:
	/** Update CRC state with a single byte */
	inline void Update(unsigned char data)
	{
		State += data;
	}

	/** Override function - we return twos complement of state */
	Hash_t Get()
	{
		return (Hash_t)(-((int8_t)State));
	}

};


#endif
