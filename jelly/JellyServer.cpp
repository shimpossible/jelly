#include "JellyServer.h"
#include "teDataChain.h"

Net::ops JellyServer::s_NetOps =
{
	JellyServer::net_state_changed,
	JellyServer::net_accepted,
	JellyServer::net_received,
};

Net::ops JellyServer::s_ClientNetOps =
{
	JellyServer::net_state_changed,
	JellyServer::net_accepted,
	JellyServer::net_received,
};

JellyServer::ServerSocketMap  JellyServer::s_KnownConnections;
JellyServer::ClientSocketMap  JellyServer::s_Clients;

JellyServer::JellyServer()
	: m_ID(JellyID::Create() )
{
}

bool JellyServer::ConnectTo(const char* address)
{
	Net::Socket_Id id = Net::connect_to(address, &s_NetOps);

	if(id == Net::InvalidSocket ) return false;

	JellyConnection* client = new JellyConnection(id, this);
	s_Clients[id] = client;

	return true;
}
bool JellyServer::Start(JELLY_U16 port)
{
	Net::Socket_Id id = Net::open_and_listen(port, &s_NetOps);
	if (id == Net::InvalidSocket ) return false;

	s_KnownConnections[id] = this;


	return true;
}
/**
	Sends message to specific object on remote server
	@param server Remote service to send to
	@param object id of object on remote server
	@param msg    message to send
	*/
JELLY_RESULT JellyServer::Send(JellyID server, ObjectID objectId, JellyMessage* msg)
{
	if(this->m_ID == server) // send to self?
	{
		m_RouteConfig.Route(m_Link, msg);
	}
	else
	{
		// send to call connections
		ConnectionMap::iterator found = m_Connections.find(server);
		if(found != m_Connections.end())
		{
			return found->second->Send(objectId, msg);
		}

	}
	// route to local objects..
	// m_RouteConfig.Route(m_Link, msg);
	return JELLY_NOT_FOUND;
}

JELLY_RESULT JellyServer::Send(ObjectID obj, JellyMessage* msg)
{
	msg->m_Destination = obj;

	//NOTE: this seems rather less than optimal..
	// would be more efficient to serialize the message first and pass
	// the serialized bytes to a connection.. but not all connections would
	// serialize to the same type.. ie BINARY, JSON, etc...
	for(ConnectionMap::iterator it = m_Connections.begin();
		it != m_Connections.end();
		it++)
	{
		(*it).second->Send(obj, msg);
	}

	// no need to send to local server, since local should have a connection to itself
	//	m_RouteConfig.Route(m_Link, msg);

	return JELLY_OK;
}

/**
	Dispatch messages found in the queue
	@param queue   Queue of messages that were received in the past
	@param handler Handler to dispatch messages to
	*/
void JellyServer::ProcessQueue(JellyMessageQueue* queue, void* handler)
{
	JellyLink link;
	JellyMessage* msg;
	while(queue->Empty() == false)
	{
		queue->Dequeue(link, msg);
		m_RouteConfig.Route(link, msg, handler);

		// Release message
		msg->Release();
	}
}


void JellyServer::net_state_changed(Net::Socket_Id id, Net::State state)
{
	switch(state)
	{
		case Net::CONNECTED:
			JellyConnection* client = FindClient(id);
			AddPending(id,client);
		break;
	}
}

void JellyServer::net_accepted(Net::Socket_Id id, Net::Socket_Id other, Net::Address& address)
{
	JellyServer* self = Find(id);
	if(self == 0)
	{
		printf("Unknown socket id: %d\r\n", id);
		return;
	}

	JellyConnection* client = new JellyConnection(other, self);
	//client->SetServer(self);
	AddPending(other,client);
}

void JellyServer::net_received(Net::Socket_Id id, void* data, size_t len)
{
	// Should be receving data from the client
	JellyConnection* client = FindClient(id);
	if(client == 0)
	{
		printf("Unknown socket %d\r\n", id);
		return ;
	}
	printf("socket %d received data %d\r\n", id, len);
	client->Receive(data,len);

}

JellyConnection* JellyServer::FindClient(Net::Socket_Id id)
{
	ClientSocketMap::iterator it = s_Clients.find(id);
	if(it != s_Clients.end())
	{
		return it->second;
	}
	return 0;
}

JellyServer* JellyServer::Find(Net::Socket_Id id)
{
	ServerSocketMap::iterator it = s_KnownConnections.find(id);
	if(it != s_KnownConnections.end())
	{
		return it->second;
	}
	return 0;
}

static void __send(void* data, size_t len, void* ctx)
{
	Net::Socket_Id id = *(Net::Socket_Id*)ctx;
	Net::send(id, (const char*) data, len,0);
}

void JellyServer::AddPending(Net::Socket_Id id, JellyConnection* client)
{
	JellyServer::s_Clients[id] = client;
	// start negotiation with other box

	char buffer[4096];
	teDataChain chain(buffer, sizeof(buffer));
	teBinaryEncoder encoder ( &chain );
	client->SendConnectRequest(&encoder);

	// flush out the chain
	Net::send(client->m_Socket, &chain, 0);
}