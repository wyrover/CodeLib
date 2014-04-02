#pragma once
#include "Common.h"
#include "IFileMap.h"

namespace CODELIB
{
    class CFileMapImpl : public IFileMap
    {
    public:
        CFileMapImpl();
        virtual ~CFileMapImpl();

        BOOL Create(LPCTSTR lpszFileName, DWORD dwProtect = PAGE_READONLY);
        void Close();
        LPVOID GetBuffer();
        DWORD GetFileSize();
    private:
        HANDLE m_hFileMap;
        LPVOID m_lpMapBuf;
        DWORD m_dwFileMapSize;
    };
}