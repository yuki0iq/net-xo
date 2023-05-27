#include "CritSect.h"

#ifdef _WIN32

#else
  #include <sys/types.h>
  #include <sys/stat.h>
  //
  #include <fcntl.h>
  #include <unistd.h>
#endif // def _WIN32

#include <string.h>
#include <stdio.h>
#include <string>
using namespace std;


//=============================================================================
CritSect& CritSect::GetCritSectInstance()
{
	static CritSect critSect;
	return critSect;
}


//=============================================================================
CritSect::CritSect()
#ifdef _WIN32
#else
	//: m_fd(-1)
#endif // def _WIN32
{
#ifdef _WIN32
	::InitializeCriticalSection(&m_cs);
#else
#endif // def _WIN32
}


//=============================================================================
CritSect::~CritSect()
{
#ifdef _WIN32
	::DeleteCriticalSection(&m_cs);
#else
	/*if(-1 != m_fd)
	{
		close(m_fd);
		m_fd = -1;
	}
	if( ! m_sFileName.isEmpty())
	{
		unlink(m_sFileName.s_str());
	}*/
#endif // def _WIN32
}


//=============================================================================
bool CritSect::Init()
{
	bool bRetCode = false;

#if _WIN32

	bRetCode = true;

#else

	/*m_sFileName = GetTmpFileName();
	m_fd = open(m_sFileName.c_str(), O_WRONLY | O_CREAT, S_IWRITE | S_IWGRP | S_IWOTH);
	bRetCode = (m_fd < 0 ? false : true);*/
	bRetCode = true;

#endif // def _WIN32

	return bRetCode;
}


//=============================================================================
void CritSect::Enter()
{
#ifdef _WIN32

	::EnterCriticalSection(&m_cs);

#else

// NOTE! :
// lockf set lock on file (BY PROCESS). Another call lockf in the same process
// return success, call lockf in other process return failed (file is locked).

//	lockf(m_fd, F_LOCK, 0); // lock all file
	lcs.enter(/*pthread_self()*/);

#endif // def _WIN32
}


//=============================================================================
void CritSect::Leave()
{
#ifdef _WIN32

	::LeaveCriticalSection(&m_cs);

#else

//	lockf(m_fd, F_ULOCK, 0); // unlock all file
	lcs.leave();

#endif // def _WIN32
}


#ifndef _WIN32
//=============================================================================
#endif // def _WIN32
