#pragma once
#include "Common.h"

namespace CODELIB
{
    enum INTEGRITYLEVEL
    {
        INTEGRITY_UNKNOWN,
        LOW_INTEGRITY,
        MEDIUM_INTEGRITY,
        HIGH_INTEGRITY,
    };

    class IProcess
    {
    public:
        virtual ~IProcess() = 0 {};
        virtual BOOL Open(DWORD dwPID) = 0;
        virtual void Close() = 0;
        virtual BOOL Terminate() = 0;
        virtual BOOL ReadMemory(LPCVOID lpBase, LPVOID lpBuf, DWORD dwWantRead) = 0;
        virtual BOOL WriteMemory(LPVOID lpBase, LPCVOID lpBuf, DWORD dwWantWrite) = 0;
        virtual DWORD GetPID() = 0;
        virtual LPCTSTR GetFullPathName() = 0;
        virtual BOOL GetIntegrityLevel(INTEGRITYLEVEL* pLevel) = 0;
        virtual HANDLE GetHandle() = 0;
    };
}