#include "stdafx.h"
#include "MiniDumpImpl.h"
#include <imagehlp.h>
#pragma comment(lib,"DbgHelp.lib")

namespace CODELIB
{
    CMiniDumpImpl::CMiniDumpImpl()
    {

    }

    CMiniDumpImpl::~CMiniDumpImpl()
    {

    }

    void CMiniDumpImpl::Active(BOOL bEnable/*=TRUE */)
    {
        if(bEnable)
            SetUnhandledExceptionFilter(&CMiniDumpImpl::MyUnhandledExceptionFilter);
    }

    LONG WINAPI CMiniDumpImpl::MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
    {
        TCHAR sModuleName[MAX_PATH] = {0};
        GetModuleFileName(NULL, sModuleName, MAX_PATH);
        TCHAR* pChar = _tcsrchr(sModuleName, _T('\\'));

        if(NULL != pChar)
        {
            int iPos = pChar - sModuleName;
            sModuleName[iPos + 1] = _T('\0');
        }

        TCHAR sFileName[MAX_PATH] = {0};
        SYSTEMTIME  systime = {0};
        GetLocalTime(&systime);
        _stprintf_s(sFileName, _T("%04d%02d%02d-%02d%02d%02d.dmp"),
                    systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);

        _tcscat_s(sModuleName, sFileName);
        HANDLE lhDumpFile = CreateFile(sModuleName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL , NULL);
        MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
        loExceptionInfo.ExceptionPointers = ExceptionInfo;
        loExceptionInfo.ThreadId = GetCurrentThreadId();
        loExceptionInfo.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);
        CloseHandle(lhDumpFile);

        return EXCEPTION_EXECUTE_HANDLER;
    }

}