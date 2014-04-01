#include "stdafx.h"
#include "IniFileImpl.h"
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

namespace CODELIB
{
    CIniFileImpl::CIniFileImpl()
    {
        ZeroMemory(m_sIniFileName, MAX_PATH);
    }

    CIniFileImpl::~CIniFileImpl()
    {

    }

    BOOL CIniFileImpl::Open(LPCTSTR lpszFileName)
    {
        if(!PathFileExists(lpszFileName))
            return FALSE;

        if(NULL == lpszFileName)
            return FALSE;

        memcpy_s(m_sIniFileName, MAX_PATH, lpszFileName, _tcslen(lpszFileName)*sizeof(TCHAR));
        return TRUE;
    }

    void CIniFileImpl::Close()
    {

    }

    DWORD CIniFileImpl::ReadDword(LPCTSTR Section, LPCTSTR key)
    {
        return GetPrivateProfileInt(Section, key, 0, m_sIniFileName);
    }

    DOUBLE CIniFileImpl::ReadDouble(LPCTSTR Section, LPCTSTR key)
    {
        TCHAR sValue[MAX_PATH] = {0};
        ReadString(Section, key, sValue);
        return _tstof(sValue);
    }

    DWORD CIniFileImpl::ReadString(LPCTSTR Section, LPCTSTR key, TCHAR* Buf)
    {
        DWORD dwReaded = GetPrivateProfileString(Section, key, 0, Buf, MAX_PATH, m_sIniFileName);
        return dwReaded + 1;
    }

    BOOL CIniFileImpl::WriteDword(LPCTSTR Section, LPCTSTR key, DWORD Value)
    {
        TCHAR sValue[MAX_PATH] = {0};
        _stprintf_s(sValue, MAX_PATH, _T("%u"), Value);
        return WritePrivateProfileString(Section, key, sValue, m_sIniFileName);
    }

    BOOL CIniFileImpl::WriteDouble(LPCTSTR Section, LPCTSTR key, double Value)
    {
        TCHAR sValue[MAX_PATH] = {0};
        _stprintf_s(sValue, MAX_PATH, _T("%f"), Value);
        return WritePrivateProfileString(Section, key, sValue, m_sIniFileName);
    }

    BOOL CIniFileImpl::WriteString(LPCTSTR Section, LPCTSTR key, LPCTSTR Value)
    {
        return WritePrivateProfileString(Section, key, Value, m_sIniFileName);
    }



}