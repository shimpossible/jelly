#pragma once

class JellyDispatchCommand
{
public:
	virtual void Dispatch(JellyLink&, JellyMessage*) = 0;
	void* TargetObject() { return obj; }

protected:
	void* obj;
};


template<typename T>
class JellyServiceDispatchCommand : public JellyDispatchCommand
{
	void (T::* fptr)(JellyLink&, JellyMessage*);
public:
	JellyServiceDispatchCommand(T* o, void (T::* dispatch)(JellyLink&, JellyMessage*))
	{
		obj = o;
		fptr = dispatch;
	}

	virtual void Dispatch(JellyLink& link, JellyMessage* msg)
	{
		T* t = (T*)obj;
		(t->*fptr)(link, msg);
	}
};

template<class LOCK_T, typename T>
class JellyServiceLockableDispatchCommand : public JellyDispatchCommand
{
	LOCK_T& m_Lock;
	void (T::* fptr)(JellyLink&, JellyMessage*);
public:
	JellyServiceLockableDispatchCommand(T* o, void (T::* dispatch)(JellyLink&, JellyMessage*), LOCK_T& lock)
		: m_Lock(lock)
	{

		obj = o;
		fptr = dispatch;
	}

	virtual void Dispatch(JellyLink& link, JellyMessage* msg)
	{
		m_Lock.Lock(msg, 0);
		T* t = (T*)obj;
		(t->*fptr)(link, msg);
		m_Lock.Unlock(msg);
	}
};
