// CodeLib.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Common.h"
#include <tchar.h>
#include <conio.h>
#include <xmemory>
using  namespace CODELIB;

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <locale.h>
#include "IIPCInterface.h"
#include "src\NamedPipeServerImpl.h"
#include "src\FileScan.h"

#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif

typedef struct _USER_DATA_PACKAGE
{
    _USER_DATA_PACKAGE(DWORD PID, LPVOID lpBuf, DWORD dwBufSize)
    {
        dwPackageType = PID;

        if(NULL != lpBuf && 0 != dwBufSize && dwBufSize <= MAX_PATH)
            memcpy_s(lpUserBuf, MAX_PATH, lpBuf, dwBufSize);

        dwTotalSize = sizeof(_USER_DATA_PACKAGE) - MAX_PATH + dwBufSize;
    }
    DWORD dwPackageType;
    BYTE lpUserBuf[MAX_PATH];
    DWORD dwTotalSize;
} USER_DATA_PACKAGE, *LPUSER_DATA_PACKAGE;

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

void TestThread()
{
    IThread* pThread = (IThread*)CreateInstance(CODELIB_THREAD);

    if(NULL != pThread)
    {
        pThread->Start();
        pThread->Stop();
        delete pThread;
        pThread = NULL;
    }
}

class CLPCEvent: public ILPCEvent
{
public:
    CLPCEvent()
    {

    }

    virtual ~CLPCEvent()
    {

    }

    void OnCreate(ILPC* pLPC)
    {

    }

    void OnClose(ILPC* pLPC)
    {

    }

    BOOL OnConnect(ILPC* pLPC, ISender* pSender)
    {
        _tsetlocale(LC_ALL, _T("chs"));
        _tprintf_s(_T("客户端 %u 连接到服务器\r\n"), pSender->GetSID());
        return TRUE;
    }

    void OnDisConnect(ILPC* pLPC, ISender* pSender)
    {
        _tsetlocale(LC_ALL, _T("chs"));
        _tprintf_s(_T("客户端 %u 从服务器断开\r\n"), pSender->GetSID());
    }

    void OnRecvAndReply(ILPC* pLPC, ISender* pSender, IMessage* pReceiveMsg, IMessage* pReplyMsg)
    {
        if(NULL != pReceiveMsg)
        {
            DWORD dwBufSize = 0;
            _tsetlocale(LC_ALL, _T("chs"));
            _tprintf_s(_T("%s"), pReceiveMsg->GetBuffer(dwBufSize));
        }

        if(pReceiveMsg->GetMessageType() == 1)
        {
            if(NULL != pReplyMsg)
            {
                TCHAR sRequest[256] = {0};
                _stprintf_s(sRequest, _T("客户端你好,我是服务端 %u"), GetCurrentProcessId());
                pReplyMsg->SetBuffer((LPVOID)sRequest, (DWORD)_tcslen(sRequest)*sizeof(TCHAR));
            }
        }
    }

    void OnRecv(ILPC* pLPC, ISender* pSender, IMessage* pMessage)
    {
        DWORD dwBufSize = 0;
        _tsetlocale(LC_ALL, _T("chs"));
        _tprintf_s(_T("%s"), pMessage->GetBuffer(dwBufSize));
    }
};

void TestLPC()
{
    ILPCEvent* pEvent = new CLPCEvent;
    std::auto_ptr<ILPCEvent> autoEvent(pEvent);
    std::auto_ptr<ILPC> pServer((ILPC*)CreateInstance(CODELIB_LPCSERVER, pEvent));
    const TCHAR* sPortName = _T("\\ServerLPC");

    if(pServer->Create(sPortName))
    {
        _getch();
        pServer->Close();
    }
}

void TestPLCClient()
{
    ILPCEvent* pEvent = new CLPCEvent;
    std::auto_ptr<ILPCEvent> autoEvent(pEvent);
    std::auto_ptr<ILPC> pServer((ILPC*)CreateInstance(CODELIB_LPCCLIENT, pEvent));
    const TCHAR* sPortName = _T("\\ServerLPC");

    if(pServer->Create(sPortName))
    {
        _getch();
        pServer->Close();
    }
}

//////////////////////////////////////////////////////////////////////////

BOOL g_bExit = FALSE;
class CNamedPipeEvent : public IIPCEvent
{
public:
    CNamedPipeEvent()
    {

    }

    virtual ~CNamedPipeEvent()
    {

    }

    virtual void OnRequest(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize)
    {
        if(NULL == lpBuf || dwBufSize == 0)
            return ;

        LPUSER_DATA_PACKAGE userRequest = (LPUSER_DATA_PACKAGE)lpBuf;
        _tsetlocale(LC_ALL, _T("chs"));
        _tprintf_s(_T("%s"), userRequest->lpUserBuf);


        if(userRequest->dwPackageType == 100)
        {
            TCHAR sReply[MAX_PATH] = {0};
            _stprintf_s(sReply, _T("同步应答：1+1=2 \r\n"));

            _USER_DATA_PACKAGE userRequest(100, sReply, (_tcslen(sReply) + 1)*sizeof(TCHAR)) ;

            pClient->PostMessage(&userRequest, sizeof(_USER_DATA_PACKAGE));
        }
        else
        {
            TCHAR sReply[MAX_PATH] = {0};
            _stprintf_s(sReply, _T("异步应答：你好，客户端 \r\n"));

            _USER_DATA_PACKAGE userRequest(1, sReply, (_tcslen(sReply) + 1)*sizeof(TCHAR)) ;

            pClient->PostMessage(&userRequest, sizeof(_USER_DATA_PACKAGE));
        }

    }
};

DWORD __stdcall PostThread(LPVOID lpParam)
{
    IIPCObject* pServer = (IIPCObject*)lpParam;

    IIPCConnectorIterator* pClientIterator = pServer->GetClients();

    while(FALSE == g_bExit)
    {
        for(pClientIterator->Begin(); !pClientIterator->End(); pClientIterator->Next())
        {
            IIPCConnector* aClient = pClientIterator->GetCurrent();

            if(NULL == aClient)
                continue;

            TCHAR* sRequest = _T("异步消息：你好，客户端端\r\n");
            DWORD dwReplySize = (DWORD)(_tcslen(sRequest) + 1) * sizeof(TCHAR);
            _USER_DATA_PACKAGE userRequest(1, sRequest, dwReplySize);

            aClient->PostMessage(&userRequest, userRequest.dwTotalSize);
        }

        Sleep(10);
    }

    return 0;
}


void TestRequestAndReply(IIPCObject* pNamedPipeClient)
{
    IIPCConnectorIterator* pClientIterator = pNamedPipeClient->GetClients();

    while(FALSE == g_bExit)
    {
        for(pClientIterator->Begin(); !pClientIterator->End(); pClientIterator->Next())
        {
            IIPCConnector* aClient = pClientIterator->GetCurrent();

            if(NULL == aClient)
                continue;

            TCHAR* sRequest = _T("1+1=2\r\n");
            DWORD dwRequestSize = (DWORD)_tcslen(sRequest) * sizeof(TCHAR);
            DWORD dwTransSize = 0;

            TCHAR sReply[MAX_PATH] = {0};

            if(aClient->RequestAndReply(sRequest, dwRequestSize, sReply, MAX_PATH))
            {
                _tsetlocale(LC_ALL, _T("chs"));
                _tprintf_s(_T("%s"), sReply);
            }
        }
    }
}

void TestNamedPipeServer()
{
    IIPCEvent* pEvent = new CNamedPipeEvent;
    IIPCObject* pNamedPipeServer = new CNamedPipeServerImpl(pEvent);

    if(NULL == pNamedPipeServer)
        return ;

    if(!pNamedPipeServer->Create(_T("NamedPipeServer")))
        return ;

    _getch();

    pNamedPipeServer->Close();
    delete pNamedPipeServer;
    pNamedPipeServer = NULL;
    delete pEvent;
    pEvent = NULL;
}

void TestFileScan()
{
// 	const TCHAR* sDriverName=_T("\\\\.\\C:");
// 	CNtfsVolumeParse diskVolume(sDriverName);
// 	if (diskVolume.OpenVolume())
// 	{
// 		ULONGLONG fileTotalCount=diskVolume.GetRecordTotalSize();
// 		if (!diskVolume.ScanVolume())
// 		{
// 			_tprintf_s(_T("文件扫描失败\r\n"));
// 		}
// 		diskVolume.CloseVolume();
// 	}

	CFileScan fileScan;
	fileScan.ExecCommand(VOLUME_FS_SCANFILECHANGE);
}

int _tmain(int argc, _TCHAR* argv[])
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//    TestProcess();
//    TestIniFile();
//    TestFileMap();
//    TestMiniDump();
//    TestThread();
//    TestLPC();
//    TestNamedPipeServer();
	TestFileScan();
    return 0;
}

