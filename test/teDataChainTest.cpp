#include "test.h"
#include "../jelly/JellyMessage.h"

namespace
{
class teDataChainTest : public ::testing::Test
{
};

TEST_F(teDataChainTest, CTOR) 
{
	teDataChain chain;
	ASSERT_EQ(0,chain.Size());
}
TEST_F(teDataChainTest, AddHead) 
{
	teDataChain chain;
	/*
	teDataChainNode node1;
	teDataChainNode node2;
	teDataChainNode node3;

	chain.AddHead( &node3 );
	chain.AddHead( &node2 );
	chain.AddHead( &node1 );

	ASSERT_EQ( &node1, chain.m_Head.next );
	ASSERT_EQ( &node2, chain.m_Head.next->next );
	ASSERT_EQ( &node3, chain.m_Head.next->next->next );

	ASSERT_EQ( &node3, chain.m_Head.prev );
	ASSERT_EQ( &node2, chain.m_Head.prev->prev );
	ASSERT_EQ( &node1, chain.m_Head.prev->prev->prev );
	*/
}

TEST_F(teDataChainTest, AddTail) 
{
	teDataChain chain;

	int tmp1,tmp2,tmp3;
	chain.AddTail( &tmp1, sizeof(tmp1) );
	chain.AddTail( &tmp2, sizeof(tmp2) );	
	chain.AddTail( &tmp3, sizeof(tmp3) );

	/*
	ASSERT_EQ( &node1, chain.m_Head.next );
	ASSERT_EQ( &node2, chain.m_Head.next->next );
	ASSERT_EQ( &node3, chain.m_Head.next->next->next );

	ASSERT_EQ( &node3, chain.m_Head.prev );
	ASSERT_EQ( &node2, chain.m_Head.prev->prev );
	ASSERT_EQ( &node1, chain.m_Head.prev->prev->prev );
	*/
}

TEST_F(teDataChainTest, Size) 
{
	teDataChain chain;

	char tmp1[10];
	char tmp2[20];
	char tmp3[30];

	chain.AddTail( tmp1, sizeof(tmp1) );
	ASSERT_EQ(10, chain.Size() );

	chain.AddTail( tmp2, sizeof(tmp2) );
	ASSERT_EQ(30, chain.Size() );

	chain.AddTail( tmp3, sizeof(tmp3) );
	ASSERT_EQ(60, chain.Size() );

}

TEST_F(teDataChainTest, Shift) 
{
	teDataChain chain;

	char tmp1[10];
	char tmp2[20];
	char tmp3[30];

	for(int i=0;i<10;i++) tmp1[i] = i;
	for(int i=0;i<20;i++) tmp2[i] = i+100;
	for(int i=0;i<30;i++) tmp3[i] = i+200;

	chain.AddTail( tmp1, sizeof(tmp1) );
	chain.AddTail( tmp2, sizeof(tmp2) );	
	chain.AddTail( tmp3, sizeof(tmp3) );

	unsigned char tmp[100];

	chain.Shift(tmp, 11);
	ASSERT_EQ(60-11, chain.Size() );
	ASSERT_EQ(9, tmp[9] );
	ASSERT_EQ(100, tmp[10] );


	chain.Shift(tmp, 11);
	ASSERT_EQ(60-11-11, chain.Size() );
	ASSERT_EQ(101, tmp[0] );
	ASSERT_EQ(111, tmp[10] );
}

TEST_F(teDataChainTest, CopyTo) 
{
	teDataChain chain;

	char tmp1[10];
	char tmp2[20];
	char tmp3[30];

	for(int i=0;i<10;i++) tmp1[i] = i;
	for(int i=0;i<20;i++) tmp2[i] = i+100;
	for(int i=0;i<30;i++) tmp3[i] = i+200;

	chain.AddTail( tmp1, sizeof(tmp1) );
	chain.AddTail( tmp2, sizeof(tmp2) );	
	chain.AddTail( tmp3, sizeof(tmp3) );

	unsigned char tmp[100];

	chain.CopyTo(tmp, 11);	
	ASSERT_EQ(60, chain.Size() );
	ASSERT_EQ(0, tmp[0] );
	ASSERT_EQ(9, tmp[9] );
	ASSERT_EQ(100, tmp[10] );


	chain.CopyTo(tmp, 22);
	ASSERT_EQ(60, chain.Size() );

	ASSERT_EQ(0, tmp[0] );
	ASSERT_EQ(9, tmp[9] );
	ASSERT_EQ(100, tmp[10] );
	ASSERT_EQ(101, tmp[11] );
	ASSERT_EQ(111, tmp[21] );
}

}