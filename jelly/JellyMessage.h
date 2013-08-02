#ifndef __JELLY_MESSAGE_H__
#define __JELLY_MESSAGE_H__

#include <queue>
#include <list>
#include "JellyMessageQueue.h"
#include "JellyRouteConfig.h"



/**
  Chains blocks of memory together 
 */
class teDataChain
{
public:
	teDataChain()
	{
		// empty state points to ourself
		m_Head.next = &m_Head;
		m_Head.prev = &m_Head;
		m_Size = 0;
	}
	~teDataChain()
	{
		// free memory
		teDataChainNode* node = m_Head.next;
		while(node != &m_Head)
		{
			teDataChainNode* next = node->next;
			delete node;
			node = next;
		}
	}


	void ForEach( void (*fptr)(void* data, size_t len, void* ctx), void* ctx)
	{
		teDataChainNode* node = m_Head.next;
		while(node != &m_Head)
		{
			fptr( node->start, node->end - node->start, ctx);
			node = node->next;
		}
	}
	size_t Size()
	{
		return m_Size;
	}

	void Clear()
	{
		teDataChainNode* last = m_Head.prev;
		teDataChainNode* curr = &m_Head;
		while(curr != last)
		{
			teDataChainNode* n = curr->next;
			delete(curr);
			curr = n;
		}
	}

	void AddTail(teDataChain* other)
	{
		teDataChainNode* node = other->m_Head.next;		
		node->prev = m_Head.prev;
		m_Head.prev->next = node;

		other->m_Head.prev->next = &m_Head;
		m_Head.prev = other->m_Head.prev;

		m_Size += other->Size();
		
		other->m_Head.next = &other->m_Head;
		other->m_Head.prev = &other->m_Head;
	}

	/**
	 Add node to the END of the chain
	*/
	void AddTail(const void* data, size_t len)//teDataChainNode* node)
	{
		teDataChainNode* node = new teDataChainNode(len);
		memcpy(node->b_start, data, len);
		// m_Head->prev is TAIL
		// m_Head->next is HEAD
		node->next = &m_Head;
		node->prev = m_Head.prev;
		m_Head.prev = m_Head.prev->next = node;		

		m_Size += node->length;
	}

	void AddHead(void* data, size_t len)//teDataChainNode* node)
	{
		teDataChainNode* node = new teDataChainNode(len);
		memcpy(node->b_start, data, len);

		// m_Head->prev is TAIL
		// m_Head->next is HEAD

		node->next = m_Head.next;
		node->prev =  &m_Head;
		m_Head.next = m_Head.next->prev = node;

		m_Size += node->length;
	}

	/**
	 Ready byte at index @index
	 @param index index to read 
	 */
	/*
	JELLY_U8 operator[](int index)
	{
		int from=0;
		int to=0;
		teDataChainNode* curr = m_Head.next;
		while(curr != &m_Head)
		{
			from  = to;
			to   += curr->length;
			if(to < index)
				curr = curr->next;
			else
				break;
		}

		if(to >= index)
		{
			return curr->start[index-from];
		}
		return 0;
	}
	*/

	/**
	 Shift bytes off the front of the buffer
	 */
	void Shift(void* _dst, size_t len)
	{
		JELLY_U8* dst = (JELLY_U8*)_dst;
		size_t leftToRead = len;
		do
		{
			size_t toRead = m_Head.next->length > leftToRead? leftToRead:  m_Head.next->length;			
			m_Head.next->Shift(dst,leftToRead);
			dst += toRead;
			leftToRead -= toRead;
			// used all the bytes in the buffer?
			if(m_Head.next->length == 0)
			{
				// remove from list
				Remove(m_Head.next);
			}
		}while(leftToRead > 0 && m_Head.next != &m_Head);

		if(len < m_Size)
		{
			m_Size -= len;
		}else
		{
			m_Size = 0;
		}
	}

	void CopyTo(void* _dst, size_t len)
	{
		JELLY_U8* dst = (JELLY_U8*)_dst;
		size_t leftToRead = len;
		teDataChainNode* curr = m_Head.next;
		while(leftToRead > 0 && curr!= &m_Head)
		{
			size_t toRead = curr->length > leftToRead? leftToRead:  curr->length;			
			curr->CopyTo(dst,leftToRead);
			dst += toRead;
			leftToRead -= toRead;
			curr = curr->next;
		}
	}

protected:
	class teDataChainNode
	{
	public:
		teDataChainNode* next;
		teDataChainNode* prev;
		JELLY_U8*        start;
		JELLY_U8*        end;
		JELLY_U8*        b_start;
		JELLY_U8*        b_end;
		size_t           length;

		teDataChainNode()
		{
			length = 0;
			start = end = nullptr;
			b_start = b_end = nullptr;
			next = prev = nullptr;
		}
		teDataChainNode(size_t length)
		{
			next = prev =nullptr;
			b_start = start = new JELLY_U8[length];
			b_end = end = start + length;

			this->length = length;
		}

		~teDataChainNode()
		{
			if(b_start)
			{
				delete[] b_start;
				length =0;
				b_start =0;
			}
		}

		void CopyTo(JELLY_U8* dst, size_t len)
		{
			size_t s = len>length? length: len;
			memcpy(dst, start, s);
		}

		void Shift(JELLY_U8* dst, size_t len)
		{
			size_t s = len>length? length: len;
			CopyTo(dst,len);
			length -=s;
			start += s;
		}
	};

	void Remove(teDataChainNode* node)
	{
		teDataChainNode *next = node->next;
		teDataChainNode *prev = node->prev;
		node->next = node->prev =  nullptr;
		next->prev = prev;
		prev->next = next;

	}

	teDataChainNode m_Head;
	size_t          m_Size;
};



class BinaryDecoder
{
public:
	virtual void Get(const char* field, JELLY_U8& value)=0;
	virtual void Get(const char* field, JELLY_U16& value)=0;
	virtual void Get(const char* field, JELLY_U32& value)=0;
};

class BinaryEncoder{
public:
	virtual void BeginMessage(unsigned short code,const char* name)=0;
	virtual void Put(const char* field, JellyID value)=0;
	virtual void Put(const char* field, JELLY_U8 value)=0;
	virtual void Put(const char* field, JELLY_U16 value)=0;
	virtual void Put(const char* field, JELLY_U32 value)=0;
	virtual void Put(const char* field, const char* value)=0;
	virtual void EndMessage(unsigned short code,const char* name)=0;
};
class JSONDecoder{};
class JSONEncoder{};


class teRawBinaryDecoder : public BinaryDecoder
{
public:
	teRawBinaryDecoder(teDataChain* chain)
	{
		m_Chain = chain;
	}
	virtual void Get(const char* field, JELLY_U8& value)
	{
		m_Chain->Shift(&value, sizeof(value));
	}
	virtual void Get(const char* field, JELLY_U16& value)
	{
		m_Chain->Shift(&value, sizeof(value));
	}
	virtual void Get(const char* field, JELLY_U32& value)
	{
		m_Chain->Shift(&value, sizeof(value));
	}
protected:
	teDataChain* m_Chain;
};

class teRawBinaryEncoder : public BinaryEncoder
{
public:
	teRawBinaryEncoder(teDataChain* chain)
	{
		m_Chain = chain;
	}

	virtual void BeginMessage(unsigned short code,const char* name)
	{
		printf("Message ID %04X\r\n", code);
		//teDataChain* _new = new teDataChain(sizeof(code));
		//memcpy(_new->Data(), &code, sizeof(code));
		//m_ChainEnd->Add( _new );
		//m_ChainEnd = _new;
	}

	virtual void Put(const char* field, JellyID value)
	{
		printf("Payload %s (ID): %08X\r\n", field, value);
		m_Chain->AddTail( &value, sizeof(value) );
	}

	virtual void Put(const char* field, JELLY_U8 value)
	{
		printf("Payload %s (char): %08X\r\n", field, value);
		m_Chain->AddTail( &value, sizeof(value) );
	}

	virtual void Put(const char* field, JELLY_U16 value)
	{
		printf("Payload %s (short): %08X\r\n", field, value);
		m_Chain->AddTail( &value, sizeof(value) );
	}

	virtual void Put(const char* field, JELLY_U32 value)
	{
		printf("Payload %s (int): %08X\r\n", field, value);
		m_Chain->AddTail( &value, sizeof(value) );
	}
	virtual void Put(const char* field, const char* value)
	{
		printf("Payload %s (string): %s\r\n", field, value);

		int len = strlen(value);
		m_Chain->AddTail( value, len );
	}

	virtual void EndMessage(unsigned short code,const char* name){}

protected:
	teDataChain* m_Chain;
};


/**
	Base message class.
	TODO: refcount
 */
class JellyMessage
{
	friend class JellyServer;
public:
	virtual ~JellyMessage(){}

	// TODO: also add a JELLY_PROTOCOL_TYPE enum
	virtual bool Put(teDataChain* pPayload) const
	{
		teRawBinaryEncoder encoder(pPayload);
		return Put(encoder);
	}
	virtual bool Get(BinaryDecoder& decoder)=0;
	virtual bool Get(JSONDecoder&   decoder)=0;
	virtual bool Put(BinaryEncoder& encoder) const=0;
	virtual bool Put(JSONEncoder&   encoder) const=0;
	/**
	 * CRC of the protocol this Message belongs to
	 * This is to make sure the CODE is the right one for the message
	 */
	virtual unsigned int GetProtocolCRC()=0;

	/**
	  Object to send to
	*/
	ObjectID GetDestination(){return destination;}

	virtual unsigned short GetCode()=0;

protected:

	JellyMessage()
		: destination()
	{
		
	}
	// set while deserialize?
	ObjectID destination;
};


#endif