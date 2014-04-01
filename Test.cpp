// CodeLib.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Common.h"
#include <tchar.h>
using  namespace CODELIB;

void TestProcess()
{
    IProcess* pProcess = (IProcess*)CreateInstance(CODELIB_PROCESS);

    if(NULL != pProcess)
    {
        if(pProcess->Open(1152))
        {
            LPCTSTR lpFullPathName = pProcess->GetFullPathName();
            INTEGRITYLEVEL level;

            if(!pProcess->GetIntegrityLevel(&level))
            {
                _tprintf(_T("GetIntegrityLevel Failed"));
            }

            BYTE buf[MAX_PATH] = {0};

            if(!pProcess->ReadMemory((LPCVOID)0x400000, buf, MAX_PATH))
            {
                _tprintf(_T("ReadMemory Failed"));
            }
        }

        pProcess->Close();
        delete pProcess;
        pProcess = NULL;
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    TestProcess();
    return 0;
}

