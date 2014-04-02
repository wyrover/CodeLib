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

void TestIniFile()
{
    IIniFile* pIniFile = (IIniFile*)CreateInstance(CODELIB_INIFILE);

    if(NULL == pIniFile)
        return ;

    if(pIniFile->Open(_T("H:\\Project\\CodeLib\\CodeLib\\Debug\\Test.ini")))
    {
        DWORD dwValue = pIniFile->ReadDword(_T("Hello"), _T("DWORD"));
        DOUBLE dwDouble = pIniFile->ReadDouble(_T("Hello"), _T("DOUBLE"));
        TCHAR sBuf[MAX_PATH] = {0};
        pIniFile->ReadString(_T("Hello"), _T("STRING"), sBuf);
        pIniFile->Close();
    }

    delete pIniFile;
    pIniFile = NULL;
}

void TestFileMap()
{
    IFileMap* pFileMap = (IFileMap*)CreateInstance(CODELIB_FILEMAP);

    if(NULL != pFileMap)
    {
        pFileMap->Create(_T("D:\\Program Files (x86)\\Tencent\\QQ\\QQProtect\\Bin\\QQProtect.exe"));
        DWORD dwFileSize = pFileMap->GetFileSize();
        LPVOID lpBuf = pFileMap->GetBuffer();
        pFileMap->Close();
        delete pFileMap;
        pFileMap = NULL;
    }
}

void TestMiniDump()
{
    IMiniDump* pMiniDump = (IMiniDump*)CreateInstance(CODELIB_MINIDUMP);

    if(NULL != pMiniDump)
    {
        pMiniDump->Active();
        int* x = 0;
        *x = 10;
        delete pMiniDump;
        pMiniDump = NULL;
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
//    TestProcess();
//    TestIniFile();
//    TestFileMap();
    TestMiniDump();
    return 0;
}

