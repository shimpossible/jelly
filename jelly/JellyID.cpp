#include "JellyTypes.h"
#include <rpc.h>

size_t JellyID_Hash(const JellyID& v)
{
	RPC_STATUS status;
	return UuidHash((UUID*)&v, &status);
}

JellyID JellyID::Create()
{
	JellyID uuid;
	::ZeroMemory(&uuid, sizeof(uuid));
	::UuidCreate((UUID*)&uuid);
	return uuid;
}


ObjectID ObjectID::Create()
{
	ObjectID uuid;
	::ZeroMemory(&uuid, sizeof(uuid));
	::UuidCreate((UUID*)&uuid);

	return uuid;
}


bool JellyID::operator==(const JellyID& other) const
{
	return memcmp(this, &other, sizeof(*this)) == 0;
}