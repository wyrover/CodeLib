// ClientDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "..\include\IIPCInterface.h"
#include <locale.h>
#include "..\src\NamedPipeClientImpl.h"
#include <conio.h>

typedef struct _USER_DATA_PACKAGE
{
    _USER_DATA_PACKAGE(DWORD PID, LPVOID lpBuf, DWORD dwBufSize)
    {
        dwPackageType = PID;

        if(NULL != lpBuf && 0 != dwBufSize)
            memcpy_s(lpUserBuf, MAX_PATH, lpBuf, dwBufSize);

        dwTotalSize = sizeof(_USER_DATA_PACKAGE) - MAX_PATH + dwBufSize;
    }
    DWORD dwPackageType;
    BYTE lpUserBuf[MAX_PATH];
    DWORD dwTotalSize;
} USER_DATA_PACKAGE, *LPUSER_DATA_PACKAGE;

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

        if(userRequest->dwPackageType == 1)
        {
            _tsetlocale(LC_ALL, _T("chs"));
            _tprintf_s(_T("%s"), userRequest->lpUserBuf);
        }
    }
};

DWORD __stdcall SendThread(LPVOID lpParam)
{
    IIPCObject* pServer = (IIPCObject*)lpParam;

    IIPCConnectorIterator* pClientIterator = pServer->GetClients();

    int x = 10;

    while(x > 0)
    {
        x--;

        for(pClientIterator->Begin(); !pClientIterator->End(); pClientIterator->Next())
        {
            IIPCConnector* aClient = pClientIterator->GetCurrent();

            if(NULL == aClient)
                continue;

            TCHAR* sRequest = _T("异步消息：你好，服务端\r\n");
            DWORD dwRequestSize = (_tcslen(sRequest) + 1) * sizeof(TCHAR);

            _USER_DATA_PACKAGE userRequest(1, sRequest, dwRequestSize);


            aClient->PostMessage(&userRequest, userRequest.dwTotalSize);
        }
    }

    return 0;
}

void TestRequestAndReply(IIPCObject* pNamedPipeClient)
{
    IIPCConnectorIterator* pClientIterator = pNamedPipeClient->GetClients();

    int x = 10;

    while(x > 0)
    {
        x--;

        for(pClientIterator->Begin(); !pClientIterator->End(); pClientIterator->Next())
        {
            IIPCConnector* aClient = pClientIterator->GetCurrent();

            if(NULL == aClient)
                continue;

            TCHAR* sRequest = _T("同步消息：1+1=?\r\n");
            DWORD dwRequestSize = (_tcslen(sRequest) + 1) * sizeof(TCHAR);
            _USER_DATA_PACKAGE userRequest(100, sRequest, dwRequestSize);

            _USER_DATA_PACKAGE userReply(0, NULL, 0);

            if(aClient->RequestAndReply(&userRequest, userRequest.dwTotalSize, &userReply, sizeof(_USER_DATA_PACKAGE)))
            {
                _tsetlocale(LC_ALL, _T("chs"));
                _tprintf_s(_T("%s"), userReply.lpUserBuf);
            }
        }
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    IIPCEvent* pEvent = new CNamedPipeEvent;

    IIPCObject* pNamedPipeClient = new CNamedPipeClientImpl(pEvent);

    if(NULL == pNamedPipeClient)
        return -1;

    if(!pNamedPipeClient->Create(_T("NamedPipeServer")))
        return -1;

    SendThread(pNamedPipeClient);

    TestRequestAndReply(pNamedPipeClient);

    _getch();

    pNamedPipeClient->Close();
    delete pNamedPipeClient;
    pNamedPipeClient = NULL;

    delete pEvent;
    pEvent = NULL;
    return 0;
}

