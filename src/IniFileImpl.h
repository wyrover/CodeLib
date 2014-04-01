#pragma once
#include "IIni.h"

namespace CODELIB
{
    class CIniFileImpl : public IIniFile
    {
    public:
        CIniFileImpl();
        virtual ~CIniFileImpl();

        virtual BOOL Open(LPCTSTR lpszFileName);

        virtual void Close();

        virtual DWORD ReadDword(LPCTSTR Section, LPCTSTR key);

        virtual DOUBLE ReadDouble(LPCTSTR Section, LPCTSTR key);

        virtual DWORD ReadString(LPCTSTR Section, LPCTSTR key, TCHAR* Buf);

        virtual BOOL WriteDword(LPCTSTR Section, LPCTSTR key, DWORD Value);

        virtual BOOL WriteDouble(LPCTSTR Section, LPCTSTR key, double Value);

        virtual BOOL WriteString(LPCTSTR Section, LPCTSTR key, LPCTSTR Value);
    private:
        TCHAR m_sIniFileName[MAX_PATH];

    };
}