#include "stdafx.h"
#include "ThreadWithFunc.h"

namespace CODELIB
{
	CThread::CThread( THREAD_FUNC threadFunc,LPVOID lpParam ):m_hThread(NULL)
		,m_threadFunc(threadFunc)
		,m_lpParam(lpParam)
	{
		m_hThread=::CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)_ThreadFunc,this,0,NULL);
	}

	CThread::~CThread()
	{
		WaitForSingleObject(m_hThread,INFINITE);
	}

	DWORD __stdcall CThread::_ThreadFunc( LPVOID lpParam )
	{
		CThread* pThis=static_cast<CThread*>(lpParam);
		if (NULL==pThis) return -1;

		if (NULL!=pThis->m_threadFunc)
			return pThis->m_threadFunc(pThis->m_lpParam);

		return 0;
	}

}