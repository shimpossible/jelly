#include <stdint.h>
#include "teDataChain.h"
#include <type_traits>

class teBinaryDecoder
{
public:
	teBinaryDecoder(teDataChain* chain)
	{
		m_Chain = chain;
	}

	struct MemberDecoder
	{
		//template<typename T, decltype( ((T*)0)->Get((teBinaryDecoder*)0 )) >
		template<typename T, decltype( ((T*)0)->Get(std::declval<teBinaryDecoder&>()))* = nullptr>
		static bool Decode(teBinaryDecoder& dec, T& t)
		{
			return t.Get(dec);
		}
	};

	struct GlobalDecoder
	{
		template<typename T,
			decltype( BinaryGet( std::declval<teBinaryDecoder&>(), std::declval<T&>() ))* = nullptr>
		static bool Decode(teBinaryDecoder& dec, T& t)
		{
			return BinaryGet(dec, t);
		}
	};

	struct MemcpyDecoder
	{
		template<typename T, typename = typename std::enable_if< std::is_trivial<T>::value>::type >
		static bool Decode(teBinaryDecoder& dec, T& t)
		{
			dec.m_Chain->Shift(&t, sizeof(t));
			return true;
		}
	};

	template<typename T, typename D, typename Decoder, typename... DecoderList>
	struct DecoderHelper : DecoderHelper<T, D, DecoderList...> {};

	template<typename T, typename Decoder, typename... DecoderList>
	struct  DecoderHelper<T,
		decltype(Decoder::Decode(std::declval<teBinaryDecoder&>(), std::declval<T&>() ) ),
		Decoder,
		DecoderList... >
	{
		typedef Decoder type;
	};

	template<typename T>
	bool Get(const char* name, T& val)
	{	
		using func = typename DecoderHelper<T, bool, // return type
			MemberDecoder,
			GlobalDecoder,
			MemcpyDecoder
		>::type;
		
		return func::Decode(*this, val);
	}

protected:
	teDataChain* m_Chain;
};


class teBinaryEncoder
{
public:
	teBinaryEncoder(teDataChain* chain)
	{
		m_Chain = chain;
	}


	virtual void BeginMessage(unsigned short code, const char* name)
	{
		printf("Message ID %04X\r\n", code);
		//teDataChain* _new = new teDataChain(sizeof(code));
		//memcpy(_new->Data(), &code, sizeof(code));
		//m_ChainEnd->Add( _new );
		//m_ChainEnd = _new;
	}
	virtual void EndMessage(unsigned short code, const char* name) {}

	struct MemberEncoder
	{
		template<typename T, decltype(((T*)0)->Put(std::declval<teBinaryEncoder&>()))* = nullptr>
		static bool Encode(teBinaryEncoder& enc, T& t)
		{
			return t.Put(enc);
		}
	};

	struct GlobalEncoder
	{
		template<typename T,
			decltype(BinaryPut(std::declval<teBinaryEncoder&>(), std::declval<T&>()))* = nullptr>
			static bool Encode(teBinaryEncoder& enc, const T& t)
		{
			return BinaryPut(enc, t);
		}
	};

	struct MemcpyEncoder
	{
		template<typename T, typename = typename std::enable_if< std::is_trivial<T>::value>::type >
		static bool Encode(teBinaryEncoder& enc, const T& t)
		{
			return enc.Put("", (const void*)&t, sizeof(t));
		}
	};

	template<typename T, typename D, typename Encoder, typename... EncoderList>
	struct EncoderHelper : EncoderHelper<T, D, EncoderList...> {};

	template<typename T, typename Encoder, typename... EncoderList>
	struct  EncoderHelper<T,
		decltype(Encoder::Encode(std::declval<teBinaryEncoder&>(), std::declval<T&>())),
		Encoder,
		EncoderList... >
	{
		typedef Encoder type;
	};

	template<typename T>
	bool Put(const char* name, T&& val)
	{
		using func = typename EncoderHelper<T, bool, // return type
			MemberEncoder,
			GlobalEncoder,
			MemcpyEncoder
		>::type;

		return func::Encode(*this, val);
	}

	bool Put(const char* name, const void* data, size_t len)
	{
		m_Chain->AddTail(data, len);
		return true;
	}
	teDataChain* GetChain() { return m_Chain; }
protected:
	teDataChain* m_Chain;
};


namespace std 
{
	bool BinaryPut(teBinaryEncoder& enc, std::string& str);
}