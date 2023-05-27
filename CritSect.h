#ifndef CRITSECT_H__E0B3C677_6E17_4c86_9C32_52A0F0B601FE__INCLUDED
#define CRITSECT_H__E0B3C677_6E17_4c86_9C32_52A0F0B601FE__INCLUDED

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif // def _WIN32

#include <string>

//=============================================================================
// One instanse for app.
#ifndef WIN32
class LINUXCS
{
	pthread_t pid = 0;
public:
	void enter()
	{
		if (pthread_self() == pid)
			return; //don't deadlock ourselves! we are already in lcs
		while(pid != 0)
			usleep(100);
		pid = pthread_self();
	}
	void leave()
	{
		if (pthread_self() != pid)
			return; //how can we leave something without entering it?
		pid = 0;
	}
};
#endif
class CritSect
{
public:
	static CritSect& GetCritSectInstance();

private:
	CritSect();
public:
	~CritSect();

public:
	// Must be called before use instance.
	bool Init();

public:
	void Enter();
	void Leave();

private:
#ifndef _WIN32

#endif // def _WIN32

private:
#ifdef _WIN32
	CRITICAL_SECTION m_cs;
#else
	/*int m_fd;
	std::string m_sFileName;*/
	LINUXCS lcs;
#endif // def _WIN32
};
//
#define theCritSect        ( CritSect::GetCritSectInstance() )
//
#define ENTER_GCRITSECT    theCritSect.Enter()
#define LEAVE_GCRITSECT    theCritSect.Leave()


#endif // ndef GCRITSECT_H__E0B3C677_6E17_4c86_9C32_52A0F0B601FE__INCLUDED
