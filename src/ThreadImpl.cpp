#include "stdafx.h"
#include "ThreadImpl.h"
#include <assert.h>

namespace CODELIB
{
    CThreadImpl::CThreadImpl(): m_hThread(NULL), m_hStopEvent(NULL)
    {
        m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    CThreadImpl::~CThreadImpl()
    {
        Stop();

        if(NULL != m_hStopEvent)
            CloseHandle(m_hStopEvent);
    }

    BOOL CThreadImpl::Start()
    {
        assert(NULL == m_hThread);

        if(NULL == m_hStopEvent)
            return FALSE;

        ResetEvent(m_hStopEvent);
        DWORD dwThreadId;
        m_hThread = CreateThread(NULL, 0, RunThreadFunc, this, 0, &dwThreadId);
        return (NULL != m_hThread);
    }

    BOOL CThreadImpl::Stop()
    {
        if(NULL == m_hThread)
            return TRUE;

        if(NULL != m_hStopEvent && NULL != m_hThread)
        {
            SetEvent(m_hStopEvent);
            DWORD exit_code;

            if(GetExitCodeThread(m_hThread, &exit_code) && exit_code == STILL_ACTIVE)
                WaitForSingleObject(m_hThread, INFINITE);

            CloseHandle(m_hThread);
            m_hThread = NULL;
        }

        return TRUE;
    }

    BOOL CThreadImpl::Run()
    {
        return TRUE;
    }

    DWORD WINAPI CThreadImpl::RunThreadFunc(LPVOID lpParam)
    {
        CThreadImpl* pThis = (CThreadImpl*)lpParam;

        if(NULL == pThis)
            return -1;

        pThis->Run();
        return 0;
    }

}