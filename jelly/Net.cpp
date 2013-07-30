
/**
	Networking glue code
 */
#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include "Net.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <list>


// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

// 10 outsstanding connect requests
#define BACKLOG 10
namespace Net
{
	Socket_Id InvalidSocket = INVALID_SOCKET;

	struct DnsLookup
	{
		HANDLE event;  // set when dns lookup is complete
		struct addrinfo* result;
	};

	/**
	Socket Control Block
	*/
	typedef struct SCB SCB;
	typedef struct SCB
	{
		Socket_Id    id;
		State        state;
		ops*         ops;

		void*        data;	// data for the current state

		SCB*         m_Next;
	} SCB;

	static SCB* g_KnownSockets = 0;
	static bool __init=false;

	static void notBlocking(Socket_Id sock)
	{
		// NOT blocking IO
		unsigned long not = 1;
		::ioctlsocket(sock, FIONBIO, &not);
	}

	static void addSocket(Socket_Id sock, Net::State state, ops* _ops,
		void* data=0)
	{
		SCB* si = new SCB();
		si->id = sock;
		si->state = state;		
		si->ops   = _ops;
		si->data  = data;
		si->m_Next = g_KnownSockets;
		g_KnownSockets = si;
	}

	bool init()
	{
		if(__init==false)
		{
			WSADATA wsaData;
			int iResult;
			iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
			if (iResult != 0) {
				return false;
			}
		}

		// we can only init once
		__init = true;
		return true;
	}
	void finalize()
	{
		if(__init)
		{
			WSACleanup();
		}
		__init = false;
	}

	/**
		Start listening for connections on a port
		@param port Port to listen on
	 */
	Socket_Id open_and_listen(JELLY_U16 port, ops* _ops)
	{
		int ret=0;
		// open a socket
		// bind the socket to a port
		// call 'listen' on that socket

		SOCKET sock = openSocket();
		if(sock == INVALID_SOCKET)
		{
			// log we couldn't open a socket
			return INVALID_SOCKET;
		}

		// attempt to find out which interface to listen on
		/*
		//This was fount at http://msdn.microsoft.com/en-us/library/windows/desktop/ms737593(v=vs.85).aspx
		// but doesnt appear to work :-/
		struct addrinfo hints;
		struct addrinfo* result;
		hints.ai_family   = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags    = AI_PASSIVE;

		int res = getaddrinfo("0.0.0.0", NULL, &hints, &result);
		if(res != 0)
		{
			printf("fail");
		}
		*/
		struct sockaddr_in result;
		result.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
		result.sin_family = AF_INET;
		result.sin_port = htons( port) ;

		if( (ret=::bind(sock, (sockaddr*)&result, sizeof(result) )) == SOCKET_ERROR )
		{
			// only needed for getaddrinfoB
			//freeaddrinfo(&result);
			closeSocket(sock);
			// log error;
			return INVALID_SOCKET;
		}
		// only needed for getaddrinfoB
		//freeaddrinfo(&result);

		if( (ret = ::listen(sock, BACKLOG)) == SOCKET_ERROR) 
		{
			printf("listen error: %d\r\n", WSAGetLastError() );
			// log error
			closeSocket(sock);
			return INVALID_SOCKET;
		}

		notBlocking(sock);

		// now that it is listening, add to list of sockets
		addSocket(sock, Net::LISTENING, _ops);
		if(_ops->state_changed)
		{
			_ops->state_changed(sock, Net::LISTENING);
		}
		return sock;
	}

	Socket_Id connect_to(const char* address, ops* _ops)
	{
		// copy supplied address into a temporary one for easy of use
		char temp[256];
		strcpy(temp, address);

		sockaddr_in ipAddr;
		ipAddr.sin_family = AF_INET;

		// split address and port  ADDRESS:PORT
		char* portStr = strchr(temp,':');
		if(portStr)
		{
			*portStr++ = 0;	// add a NULL so address becomes a string in itself
			ipAddr.sin_port = htons( atoi(portStr) );
		}

		Socket_Id clientSock = openSocket();
		if(clientSock == INVALID_SOCKET)
		{
			// TODO: log
			return INVALID_SOCKET;
		}
		notBlocking(clientSock);

		ipAddr.sin_family = AF_INET;
		if(inet_pton(AF_INET, temp, &ipAddr.sin_addr) == 1)
		{
			// temp was  X.X.X.X format and converted successfuly

			if(::connect( clientSock, (sockaddr*)&ipAddr, sizeof(ipAddr))  == -1)
			{
				// some error happend :-/
				int err = WSAGetLastError();
				if(err != WSAEWOULDBLOCK )
				{
					closeSocket(clientSock);
					return INVALID_SOCKET;
				}

				addSocket(clientSock, Net::PENDING_CONNECTION, _ops);
				if(_ops->state_changed)
				{
					_ops->state_changed(clientSock, Net::PENDING_CONNECTION);
				}
			}
		}else
		{
			DnsLookup* lookup = new DnsLookup();
			struct addrinfo hints;

			lookup->event = CreateEvent(NULL, TRUE,FALSE,NULL);

			// TODO: run this part in a different thread
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_family   = AF_INET;  
			if(getaddrinfo( temp, portStr, &hints, &lookup->result) != 0)
			{
				CloseHandle(lookup->event);
				delete lookup;
				// error;
				return INVALID_SOCKET;
			}
			SetEvent(lookup->event);
			// TODO: run the above part in a 2nd thread

			// temp is a DNS name and needs to be looked up
			addSocket( clientSock,  Net::DNS_LOOKUP, _ops, lookup);
			if(_ops->state_changed)
			{
				_ops->state_changed(clientSock, Net::DNS_LOOKUP);
			}
		}

		return clientSock;
	}

	Socket_Id accept(Socket_Id sock, Address* addr)
	{
		sockaddr_in clientAddr;
		socklen_t len = sizeof(clientAddr);
		SOCKET clientSocket = ::accept(sock, (sockaddr*)&clientAddr, &len);
		if(clientSocket != INVALID_SOCKET)
		{		
			addr->port    = ntohs(clientAddr.sin_port);
			addr->address = ntohl(clientAddr.sin_addr.S_un.S_addr);
		}

		return clientSocket;
	}

	bool closeSocket(Socket_Id sock)
	{
		::closesocket(sock);
		return true;
	}
	/**
	Opens a Socket for the given protocol, only supports TCP
	right now
	@param protocol  Protocol to use (TCP, UDP)
	 */
	Socket_Id openSocket()
	{
		SOCKET sock = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
		if(sock == INVALID_SOCKET)
		{
			// invalid socket?
		}

		return sock;
	}


	// forward declarations
	static void processListening(SCB* sock);
	static bool processDnsLookup(SCB* sock);
	static bool processPendingConnection(SCB* sock);
	static bool processConnected(SCB* sock);

	/**
	  Call this periodically (all the time) to process connections

	 */
	void process()
	{
		SCB* curr = g_KnownSockets;
		SCB* last = nullptr;
		
		while(curr != nullptr)
		{
			bool remove = false;
			switch(curr->state)
			{
			case Net::LISTENING:
				processListening(curr);
				remove = false;
				break;
			case Net::DNS_LOOKUP:
				remove = processDnsLookup(curr);
				break;
			case Net::PENDING_CONNECTION:
				remove = processPendingConnection(curr);
				if(remove)
				{
					printf("Pending error\r\n");
				}
				break;
			case Net::CONNECTED:
				remove = processConnected(curr);
				if(remove)
				{
					printf("CONNECTED error\r\n");
				}
				break;
			}

			if(remove)
			{
				if(last!=0) last->m_Next = curr->m_Next;

				SCB* tmp = curr;
				curr = curr->m_Next;

				delete tmp;
			}else
			{
				last = curr;
				curr = curr->m_Next;
			}
		}
	}

	// 1500 because that is the default MTU size
	static char recv_buffer[1500];
	static bool processConnected(SCB* sock)
	{		
		int buffLen = sizeof(recv_buffer);
		int bytesRead;
		bool disconnect = false;
		if(recv(sock->id, recv_buffer, buffLen, &bytesRead) != 0)
		{
			disconnect = true;
		}else
		{
			int err = WSAGetLastError();

			// no bytes available
			if(err == WSAEWOULDBLOCK) return false;

			if(bytesRead <= 0)
			{
				// there was no error, yet we got a NEGATIVE or 0 amount of data
				// means the socket is dead...
				disconnect = true;
			}else
			{
				if(sock->ops->received && bytesRead > 0)
				{
					sock->ops->received(sock->id, recv_buffer, bytesRead);
				}
			}
		}

		if(disconnect)
		{
			// error trying to read from socket.. mark for removal
			if(sock->ops->state_changed)
			{
				sock->ops->state_changed( sock->id, Net::DISCONNECTED);
			}
			return true;
		}

		return false;
	}
	static void processListening(SCB* sock)
	{
		Address addr;
		Socket_Id client = accept(sock->id, &addr);
		if(client != INVALID_SOCKET)
		{
			notBlocking(client);
			addSocket( client, Net::CONNECTED, sock->ops);
			if(sock->ops->accepted)
			{
				sock->ops->accepted(sock->id, client, addr);
			}
		}
	}

	static bool processPendingConnection(SCB* sock)
	{
		JELLY_U32 optval;
		socklen_t optlen = sizeof(optval);
		if( getsockopt( sock->id, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) == -1)
		{
			// fail
			return true;
		}else
		{
			// not connected yet...
			if(optval == WSAEINPROGRESS)
			{
				return false;
			}

			// connected
			if(optval == 0)
			{
				// can we write to this yet?

				fd_set fds;
				timeval timeout;
				FD_ZERO(&fds);
				FD_SET(sock->id, &fds);
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;

				if( ::select( sock->id +1, NULL,  &fds, NULL, &timeout) > 0 )
				{
					sock->state = Net::CONNECTED;
					if(sock->ops->state_changed)
					{
						sock->ops->state_changed(sock->id, sock->state);
					}
				}

			}else
			{
				// error?
				return true;
			}
		}

		return false;
	}

	static bool processDnsLookup(SCB* sock)
	{
		DnsLookup* lookup = (DnsLookup*)sock->data;
		if( WaitForSingleObject(lookup->event, 0) == WAIT_OBJECT_0)
		{
			// finished lookup
			CloseHandle(lookup->event);
			
			// try to connect again
			if( ::connect( sock->id, lookup->result->ai_addr, lookup->result->ai_addrlen)  )
			{
				// some error happend :-/
				int err = WSAGetLastError();
				if(err != WSAEWOULDBLOCK &&
					err != WSAEWOULDBLOCK)
				{
					// remove soket from list..
					closeSocket(sock->id);
					sock->id = 0;
					// remove from knownSocket list
					return true;
				}else
				{
					sock->state = Net::PENDING_CONNECTION;
					if(sock->ops->state_changed)
					{
						sock->ops->state_changed(sock->id, sock->state);
					}
				}
			}else
			{
				sock->state = Net::CONNECTED;
				if(sock->ops->state_changed)
				{
					sock->ops->state_changed(sock->id, sock->state);
				}
			}

			delete lookup;
			sock->data = 0;
		}

		return false;
	}


	int send(Socket_Id sock, const char* data, size_t len, int* bytesSent)
	{
		int r = ::send(sock, data, len, NULL);

		if(r == SOCKET_ERROR)
		{			
			if(bytesSent!=0) *bytesSent = 0;
			return WSAGetLastError();
		}

		if(bytesSent!=0) *bytesSent = r;		
		return 0;
	}

	int recv(Socket_Id sock, char* dst, size_t length, int* read)
	{
		int r = ::recv(sock, dst, length, NULL);
		if(r == SOCKET_ERROR)
		{
			if(read!=0) *read=0;
			int err =  WSAGetLastError();
			
			// nothing on the socket?
			if(err = WSAEWOULDBLOCK)
			{
				return 0;
			}else
			{
				return err;
			}
		}

		if(read!=0) *read=r;		
		return 0;
	}
}