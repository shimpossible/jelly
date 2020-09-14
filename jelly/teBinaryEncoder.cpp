#include "teBinaryEncoder.h"

namespace std
{
	bool BinaryPut(teBinaryEncoder& enc, std::string& str)
	{
		enc.Put("len", str.length());
		enc.GetChain()->AddTail(str.c_str(), str.length());

		return true;
	}
}