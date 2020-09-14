#ifndef __JELLYTYPES_H__
#define __JELLYTYPES_H__

typedef unsigned long long   JELLY_U64;
typedef unsigned long        JELLY_U32;
typedef unsigned short       JELLY_U16;
typedef unsigned char        JELLY_U8;

typedef signed long long   JELLY_S64;
typedef signed long        JELLY_S32;
typedef signed short       JELLY_S16;
typedef signed char        JELLY_S8;

typedef enum JELLY_RESULT
{
	JELLY_NOT_FOUND = -1,
	JELLY_OK=0,
} JELLY_RESULT;

typedef enum JELLY_MESSAGE_TYPE
{
	JELLY_MESSAGE_CONNECT,
	JELLY_MESSAGE_PING,	// ping request
	JELLY_MESSAGE_PONG,	// ping response
	JELLY_MESSAGE_OBJECT,
	JELLY_MESSAGE_SERVICE,
} JELLY_MESSAGE_TYPE;

class teBinaryEncoder;
class teBinaryDecoder;

/** 128 bit UUID */
template<unsigned int>
struct JellyGUID
{
	JELLY_U32 a;
	JELLY_U32 b;
	JELLY_U32 c;
	JELLY_U32 d;
	
	JellyGUID()
	{
		a=b=c=d=0;
	}

	static JellyGUID Create();

	bool operator==(const JellyGUID& b) const;

	bool Put(teBinaryEncoder& enc)
	{
		enc.Put("a", a);
		enc.Put("b", b);
		enc.Put("c", c);
		enc.Put("d", d);
		return true;
	}
	bool Get(teBinaryDecoder& dec)
	{
		dec.Get("a", a);
		dec.Get("b", b);
		dec.Get("c", c);
		dec.Get("d", d);
		return true;
	}
};


typedef struct JellyGUID<0> JellyID;
typedef struct JellyGUID<1> ObjectID;


size_t JellyID_Hash(const JellyID& v);
#include <unordered_map>
namespace std
{
	template<>
	struct hash<JellyID>
	{
		inline size_t operator()(const JellyID &v) const
		{
			return JellyID_Hash(v);
		}
	};
}


class JellyLink
{
public:
	const char* Describe();
	JellyID Sender();
};


class JellyMessage;

JELLY_U32 Encode7Bit(JELLY_U32 val);

#endif