#ifndef _CRC_H_
#define _CRC_H_

#include <inttypes.h>

#define CRC_LO8(x) ((unsigned char)(x&0x00FF))
#define CRC_HI8(x) (CRC_LO8(x>>8))

/** CRC16 CCITT implementation */
class CRC16 {
public:
	/** CRC16 type **/
	typedef uint16_t CRC_t;

	/** CRC state - should be read after calculation */
	CRC_t State;

	/** Initialize CRC state */
	inline void Init()
	{
		State = 0xFFFF;
	}

	CRC16()
	{
		this->Init();
	}

	/** Update CRC state with a single byte */
	inline void Update(unsigned char data)
	{
		data ^= CRC_LO8(State);
		data ^= data << 4;

		State = (
				( ((CRC_t)data << 8) | CRC_HI8(State) )
				^ (unsigned char)(data >> 4)
				^ ((CRC_t)data << 3)
		);
	}
};

/** CRC8 CCITT implementation */
class CRC8 {
public:
	/** CRC16 type **/
	typedef uint8_t CRC_t;

	/** CRC state - should be read after calculation */
	CRC_t State;

	/** Initialize CRC state */
	inline void Init()
	{
		State = 0x00;
	}

	CRC8()
	{
		this->Init();
	}

	/** Update CRC state with a single byte */
	inline void Update(unsigned char data)
	{
		uint8_t i;

		State = State ^ data;
		for (i = 0; i<8; i++)
		{
			if (State & 0x01)
				State = (State >> 1) ^ 0x8C;
			else
				State >>= 1;
		}
	}

};


#endif
