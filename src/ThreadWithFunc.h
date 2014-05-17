#pragma once
#include <windows.h>

namespace CODELIB
{
	typedef DWORD (*THREAD_FUNC)(LPVOID);
    class CThread
    {
    public:
		CThread(THREAD_FUNC threadFunc,LPVOID lpParam);
		virtual ~CThread();
	protected:
		static DWORD __stdcall _ThreadFunc(LPVOID lpParam);
    private:
		THREAD_FUNC m_threadFunc;
		LPVOID m_lpParam;
		HANDLE m_hThread;
    };

}