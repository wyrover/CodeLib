#pragma once
#include "Common.h"

namespace CODELIB
{
    class IIniFile
    {
    public:
        virtual ~IIniFile() = 0 {};
        virtual BOOL Open(LPCTSTR lpszFileName) = 0;
        virtual void Close() = 0;
        virtual DWORD ReadDword(LPCTSTR Section, LPCTSTR key) = 0;
        virtual DOUBLE ReadDouble(LPCTSTR Section, LPCTSTR key) = 0;
        virtual DWORD ReadString(LPCTSTR Section, LPCTSTR key, TCHAR* Buf) = 0;
        virtual BOOL WriteDword(LPCTSTR Section, LPCTSTR key, DWORD Value) = 0;
        virtual BOOL WriteDouble(LPCTSTR Section, LPCTSTR key, double Value) = 0;
        virtual BOOL WriteString(LPCTSTR Section, LPCTSTR key, LPCTSTR Value) = 0;
    };
}