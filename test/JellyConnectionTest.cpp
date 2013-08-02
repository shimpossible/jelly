#include "test.h"
#include "../jelly/JellyConnection.h"
#include "../jelly/JellyServer.h"

namespace
{

class Proto1
{
public:
	// computed CRC of messages
	static const unsigned CRC = 0xCAFE0001;
	static const char*    NAME;
	template<typename HANDLER_T>
	static JELLY_RESULT Dispatch(JellyLink& link, JellyMessage* pMessage,HANDLER_T* pHandler)
	{
		return JELLY_OK;
	}
	static JellyMessage* CreateMessage(JELLY_U16 msgId){ return 0;}
};
class Proto2
{
public:
	// computed CRC of messages
	static const unsigned CRC = 0xCAFE0002;
	static const char*    NAME;

	template<typename HANDLER_T>
	static JELLY_RESULT Dispatch(JellyLink& link, JellyMessage* pMessage,HANDLER_T* pHandler)
	{
		return JELLY_OK;
	}
	static JellyMessage* CreateMessage(JELLY_U16 msgId){ return 0;}
};

const char* Proto1::NAME = "Proto1";
const char* Proto2::NAME = "ABCD";

class MockHandler
{
public:
	void dispatch(JellyLink& link, JellyMessage* pMessage)
	{
	}
};

class JellyConnectionTest : public ::testing::Test
{
};


TEST_F(JellyConnectionTest, CTOR) 
{
	MockHandler handler;
	JellyServer server;
	JellyConnection conn(0, &server);

	server.GetRouteConfig().ConfigureInbound<Proto1>(&handler, &MockHandler::dispatch);
	server.GetRouteConfig().ConfigureInbound<Proto2>(&handler, &MockHandler::dispatch);


	JELLY_U8 data[1024];

	// 128bit ID
	for(int i=0;i<16;i++)
		data[i] = i;

	// length
	int idx = 16;
	data[idx+0] = 8;
	data[idx+1] = 0x00;
	data[idx+2] = 0x00;
	data[idx+3] = 0x00;

	data[idx+4] = 0x41;
	data[idx+5] = 0x42;
	data[idx+6] = 0x43;
	data[idx+7] = 0x44;

	data[idx+11]  = 0xCA;
	data[idx+10]  = 0xFE;
	data[idx+9] = 0x00;
	data[idx+8] = 0x02;

	ASSERT_EQ(-1, conn.GetChannelId(0xCAFE0002) );

	conn.Receive( data, 16+12 );

	ASSERT_EQ(0, conn.GetChannelId(0xCAFE0002) );

	// non existing channel
	ASSERT_EQ(-1, conn.GetChannelId(0xCAFE0001) );

	// doesnt add the item 2x
	conn.Receive( &data[idx], 12 );	
	ASSERT_EQ(0, conn.GetChannelId(0xCAFE0002) );

}

}