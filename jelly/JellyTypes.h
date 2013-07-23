#ifndef __JELLYTYPES_H__
#define __JELLYTYPES_H__

typedef unsigned int   JELLY_U32;
typedef unsigned short JELLY_U16;
typedef unsigned char  JELLY_U8;

typedef enum JELLY_RESULT
{
	JELLY_OK=0,
} JELLY_RESULT;

// 128bit address
struct JellyID
{
	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int d;
};

typedef unsigned int ObjectID;

class JellyLink
{
public:
	const char* Describe();
	JellyID Sender();
};


class JellyMessage;

#endif