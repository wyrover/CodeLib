#pragma once
#include "IThread.h"

namespace CODELIB
{
    class CThreadImpl : public IThread
    {
    public:
        CThreadImpl();
        virtual ~CThreadImpl();

        BOOL Start();

        BOOL Stop();

        BOOL Run();
    protected:
        static DWORD WINAPI RunThreadFunc(LPVOID lpParam);
    private:
        HANDLE m_hStopEvent;
        HANDLE m_hThread;
    };
}