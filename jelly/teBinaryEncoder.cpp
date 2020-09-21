#include "teBinaryEncoder.h"

namespace std
{
	bool BinaryPut(teBinaryEncoder& enc, std::string& str)
	{
		bool result = true;
		result = result && enc.Put("len", str.length());
		result = result && enc.GetChain()->Write(str.c_str(), str.length());
		return result;
	}

	bool BinaryGet(teBinaryDecoder& enc, std::string& str)
	{
		bool result = true;
		uint32_t len;
		result = result && enc.Get("len", len);
		str.reserve(len);
		// read directly into buffer
		result = result && enc.GetChain()->Read((char*)str.c_str(), len);
		return result;
	}
}