#include "JellyTypes.h"
#include "teBinaryEncoder.h"

bool UInt7BitEncoded::Put(teBinaryEncoder& enc)
{
	uint32_t val = value;
	uint8_t v;
	// MSB with a 1 as long as there is a new value
	while (val >= 0x80)
	{
		v = (val | 0x80) & 0xFF;
		if (!enc.Put("", v)) return false;
		val = val >> 7; // shift off the lower 7 bits
	}
	// val should be less than 0x80 at this point
	v = val;
	return enc.Put("", v);
}
bool UInt7BitEncoded::Get(teBinaryDecoder& dec)
{
	uint8_t v;
	int shift = 0;
	value = 0;
	do
	{
		if (!dec.Get("", v)) return false;

		value = value | ((v & 0x7F) << shift);
		shift += 7;
	} while (v & 0x80);
	return true;
}
