#pragma once
#include "Common.h"

namespace CODELIB
{
    class IFileMap
    {
    public:
        virtual ~IFileMap() = 0 {};
        virtual BOOL Create(LPCTSTR lpszFileName, DWORD dwProtect = PAGE_READONLY) = 0;
        virtual void Close() = 0;
        virtual LPVOID GetBuffer() = 0;
        virtual DWORD GetFileSize() = 0;
    };
}