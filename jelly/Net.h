#ifndef __NET_H__
#define __NET_H__
#include "JellyTypes.h"
#include "teDataChain.h"

namespace Net
{	
	
	typedef JELLY_U32 Socket_Id;
	
	extern Socket_Id InvalidSocket;

	typedef struct Address Address;

	typedef struct Address
	{
		JELLY_U16 port;
		JELLY_U32 address;		
	} Address;
	typedef enum State
	{
		LISTENING,
		CONNECTED,
		PENDING_CONNECTION,
		DNS_LOOKUP,
		DISCONNECTED,
	} State;
	typedef struct ops
	{
		void (*state_changed)(Socket_Id id, State state);
		void (*accepted)(Socket_Id id, Socket_Id other, Address& address);
		void (*received)(Socket_Id id, void* data, size_t len);
	} ops;
	
	bool init();
	void finalize();

	/**
	 Listen for connections 
	 @param port  Port to listen on
	 @param _ops  Callbacks for this connection
	 */
	Socket_Id open_and_listen(JELLY_U16 port, ops* _ops);
	/**
	 Connect to a server
	 @param address  IPv4 address to connect to
	 @param _ops     callbacks for this connection
	 */
	Socket_Id connect_to(const char* address, ops* _ops);

	/**
	  Sends data out the socket
	  @param sock  Socket to send on
	  @param data  Data to send
	  @param len   Length of data
	  @param bytesSent  Number of bytes sent
	  @returns error code
	 */
	int send(Socket_Id sock, const char* data, size_t len, int* bytesSent);

	/**
	  Sends blocks of data to socket
	  @param  sock   Socket to send on
	  @param  chain  Chain of buffers to send
	  @param  bytesSent   Number of bytes sent
	  @returns error code
	 */
	int send(Socket_Id sock, teDataChain* chain, int* bytesSent);

	/**
	 Read from a socket
	 @param sock   Socket to read from
	 @param dst    Destination to store
	 @param length Size of destination
	 @param read   Number of bytes read
	 @returns error code 
	 */
	int recv(Socket_Id sock, char* dst, size_t length, int* read);

	Socket_Id accept(Socket_Id* sock, Address* addr);
	bool closeSocket(Socket_Id sock);
	Socket_Id openSocket();

	/**
	 Call this to allow network thread to handle connections
	 */
	void process();

}

#endif /*__NET_H__ */