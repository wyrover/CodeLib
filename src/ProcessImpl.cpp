#include "stdafx.h"
#include "ProcessImpl.h"
#include <assert.h>
#include <Psapi.h>
#include <tlhelp32.h>

namespace CODELIB
{
    CProcessImpl::CProcessImpl(void): m_dwPID(-1), m_hProcess(NULL)
    {
    }


    CProcessImpl::~CProcessImpl(void)
    {
        Close();
    }

    BOOL CProcessImpl::Open(DWORD dwPID)
    {
        assert(NULL == m_hProcess);
        m_hProcess =::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

        if(NULL != m_hProcess)
            m_dwPID = dwPID;

        return (NULL != m_hProcess);
    }

    void CProcessImpl::Close()
    {
        if(NULL != m_hProcess)
        {
            ::CloseHandle(m_hProcess);
            m_hProcess = NULL;
        }
    }

    BOOL CProcessImpl::Terminate()
    {
        BOOL bRet = FALSE;

        if(NULL != m_hProcess)
            bRet =::TerminateProcess(m_hProcess, 0);

        return bRet;
    }

    BOOL CProcessImpl::ReadMemory(LPCVOID lpBase, LPVOID lpBuf, DWORD dwWantRead)
    {
        BOOL bRet = FALSE;
        DWORD dwReaded = 0;

        if(NULL != m_hProcess)
            bRet =::ReadProcessMemory(m_hProcess, lpBase, lpBuf, dwWantRead, (SIZE_T*)&dwReaded);

        return ((TRUE == bRet) && (dwWantRead == dwReaded));
    }

    BOOL CProcessImpl::WriteMemory(LPVOID lpBase, LPCVOID lpBuf, DWORD dwWantWrite)
    {
        BOOL bRet = FALSE;
        DWORD dwWrited = 0;

        if(NULL != m_hProcess)
            bRet =::WriteProcessMemory(m_hProcess, lpBase, lpBuf, dwWantWrite, (SIZE_T*)&dwWrited);

        return ((TRUE == bRet) && (dwWrited == dwWantWrite));
    }

    DWORD CProcessImpl::GetPID()
    {
        return m_dwPID;
    }

    LPCTSTR CProcessImpl::GetFullPathName()
    {
        ZeroMemory(m_sFullPathName, MAX_PATH);

        if(0 != GetModuleFileNameEx(m_hProcess, NULL, m_sFullPathName, MAX_PATH))
            return m_sFullPathName;

        return NULL;
    }

    BOOL CProcessImpl::GetIntegrityLevel(INTEGRITYLEVEL* pLevel)
    {
        if(!pLevel)
            return FALSE;

        HANDLE process_token;

        if(!OpenProcessToken(m_hProcess, TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                             &process_token))
            return FALSE;

        DWORD token_info_length = 0;

        if(GetTokenInformation(process_token, TokenIntegrityLevel, NULL, 0,
                               &token_info_length) ||
                GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return FALSE;

        char* tokenInfo = new char[token_info_length];

        TOKEN_MANDATORY_LABEL* token_label =
            reinterpret_cast<TOKEN_MANDATORY_LABEL*>(tokenInfo);

        if(!token_label)
            return FALSE;

        if(!GetTokenInformation(process_token, TokenIntegrityLevel, token_label,
                                token_info_length, &token_info_length))
            return FALSE;

        DWORD integrity_level = *GetSidSubAuthority(token_label->Label.Sid,
                                (DWORD)(UCHAR)(*GetSidSubAuthorityCount(token_label->Label.Sid) - 1));

        if(integrity_level < SECURITY_MANDATORY_MEDIUM_RID)
        {
            *pLevel = LOW_INTEGRITY;
        }
        else if(integrity_level >= SECURITY_MANDATORY_MEDIUM_RID &&
                integrity_level < SECURITY_MANDATORY_HIGH_RID)
        {
            *pLevel = MEDIUM_INTEGRITY;
        }
        else if(integrity_level >= SECURITY_MANDATORY_HIGH_RID)
        {
            *pLevel = HIGH_INTEGRITY;
        }
        else
        {
            return FALSE;
        }

        delete[] tokenInfo;
        tokenInfo = NULL;

        return TRUE;
    }

    HANDLE CProcessImpl::GetHandle()
    {
        return m_hProcess;
    }

    DWORD CProcessImpl::FindProcessIDByName(LPCTSTR lpszName)
    {
        DWORD dwPID = 0;
        PROCESSENTRY32 pe32 = {sizeof(pe32)};
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if(INVALID_HANDLE_VALUE == hProcessSnap) return -1;

        if(Process32First(hProcessSnap, &pe32))
        {
            do
            {
                if(0 == _tcsicmp(lpszName, pe32.szExeFile))
                {
                    dwPID = pe32.th32ProcessID;
                    break;
                }
            }
            while(Process32Next(hProcessSnap, &pe32));
        }

        CloseHandle(hProcessSnap);
        hProcessSnap = NULL;
        return dwPID;
    }

	BOOL CProcessImpl::EnumProcess( std::vector<PROCESSENTRY32>& proVec )
	{
		PROCESSENTRY32 pe32 = {sizeof(pe32)};
		HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if(INVALID_HANDLE_VALUE == hProcessSnap) return FALSE;

		if(Process32First(hProcessSnap, &pe32))
		{
			do
			{
				proVec.push_back(pe32);
			}
			while(Process32Next(hProcessSnap, &pe32));
		}

		CloseHandle(hProcessSnap);
		hProcessSnap = NULL;
		return TRUE;
	}

}

