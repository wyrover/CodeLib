#include "stdafx.h"
#include "FileMapImpl.h"
#include <shlwapi.h>

namespace CODELIB
{
    CFileMapImpl::CFileMapImpl(): m_hFileMap(NULL)
        , m_dwFileMapSize(0)
        , m_lpMapBuf(NULL)
    {

    }

    CFileMapImpl::~CFileMapImpl()
    {

    }

    BOOL CFileMapImpl::Create(LPCTSTR lpszFileName, DWORD dwProtect)
    {
        if(!PathFileExists(lpszFileName))
        {
            return FALSE;
        }

        if(NULL != m_hFileMap)
        {
            return TRUE;
        }

        HANDLE hFile =::CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(INVALID_HANDLE_VALUE == hFile)
        {
            return FALSE;
        }

        m_dwFileMapSize =::GetFileSize(hFile, 0);

        m_hFileMap =::CreateFileMapping(hFile, NULL, dwProtect, 0, 0, NULL);

        if(NULL == m_hFileMap)
        {
            CloseHandle(hFile);
            return FALSE;
        }

        m_lpMapBuf =::MapViewOfFile(m_hFileMap, FILE_MAP_READ, 0, 0, 0);

        if(NULL == m_lpMapBuf)
        {
            Close();
            CloseHandle(hFile);
            return FALSE;
        }

        CloseHandle(hFile);

        return TRUE;
    }

    void CFileMapImpl::Close()
    {
        if(NULL != m_lpMapBuf)
        {
            UnmapViewOfFile(m_lpMapBuf);
            m_lpMapBuf = NULL;
        }

        if(NULL != m_hFileMap)
        {
            CloseHandle(m_hFileMap);
            m_hFileMap = NULL;
        }

        m_dwFileMapSize = 0;
    }

    LPVOID CFileMapImpl::GetBuffer()
    {
        return m_lpMapBuf;
    }

    DWORD CFileMapImpl::GetFileSize()
    {
        return m_dwFileMapSize;
    }

}