#pragma once
#include "IProcess.h"

namespace CODELIB
{
    class CProcessImpl : public IProcess
    {
    public:
        CProcessImpl(void);
        virtual ~CProcessImpl(void);
    public:
        BOOL Open(DWORD dwPID);
        void Close();
        BOOL Terminate();
        BOOL ReadMemory(LPCVOID lpBase, LPVOID lpBuf, DWORD dwWantRead);
        BOOL WriteMemory(LPVOID lpBase, LPCVOID lpBuf, DWORD dwWantWrite);
        DWORD GetPID();
        LPCTSTR GetFullPathName();
        BOOL GetIntegrityLevel(INTEGRITYLEVEL* pLevel);
    private:
        DWORD m_dwPID;
        HANDLE m_hProcess;
        TCHAR m_sFullPathName[MAX_PATH];
    };
}


