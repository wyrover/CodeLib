#include "stdafx.h"
#include "NamedPipeServerImpl.h"

CNamedPipeServerImpl::CNamedPipeServerImpl(IIPCEvent* pEvent): m_pEvent(pEvent)
    , m_hThreadIOCP(NULL)
    , m_dwIOCPThreadNum(0)
{
    InitializeCriticalSection(&m_csClientMap);
    SYSTEM_INFO sysInfo = { 0 };
    GetNativeSystemInfo(&sysInfo);
    m_dwIOCPThreadNum = sysInfo.dwNumberOfProcessors * 2;
}

CNamedPipeServerImpl::~CNamedPipeServerImpl()
{
    Close();
    DeleteCriticalSection(&m_csClientMap);
}

BOOL CNamedPipeServerImpl::Create(LPCTSTR lpPipeName)
{
    if(NULL == lpPipeName)
        return FALSE;

    _tcscpy_s(m_sPipeName, MAX_PATH, _T("\\\\.\\pipe\\"));
    _tcscat_s(m_sPipeName, lpPipeName);

    if(!m_iocp.Create(m_dwIOCPThreadNum))
        return FALSE;

    m_hThreadIOCP = new HANDLE[m_dwIOCPThreadNum];

    if(NULL == m_hThreadIOCP)
        return FALSE;

    for(DWORD i = 0; i < m_dwIOCPThreadNum; ++i)
        m_hThreadIOCP[i] = CreateThread(0, 0, IOCompletionThread, this, 0, NULL);

    WaitClientConnect();
    return TRUE;
}

void CNamedPipeServerImpl::Close()
{
    m_iocp.Close();

    if(NULL != m_hThreadIOCP)
        WaitForMultipleObjects(m_dwIOCPThreadNum, m_hThreadIOCP, TRUE, INFINITE);

    for(DWORD i = 0; i < m_dwIOCPThreadNum; i++)
    {
        if(NULL != m_hThreadIOCP && NULL != m_hThreadIOCP[i])
        {
            CloseHandle(m_hThreadIOCP[i]);
            m_hThreadIOCP[i] = NULL;
        }
    }

    delete m_hThreadIOCP;
    m_hThreadIOCP = NULL;

    for(ConnectorMap::const_iterator cit = m_connectorMap.begin(); cit != m_connectorMap.end(); cit++)
    {
        CNamedPipeConnector* pClient = dynamic_cast<CNamedPipeConnector*>(*cit);

        if(NULL != pClient)
        {
            pClient->Close();
            delete pClient;
            pClient = NULL;
        }
    }

    m_connectorMap.clear();
}

IIPCConnectorIterator* CNamedPipeServerImpl::GetClients()
{
    return this;
}

ConnectorMap::const_iterator CNamedPipeServerImpl::FindClient(HANDLE hPipe)
{
    EnterCriticalSection(&m_csClientMap);
    ConnectorMap::const_iterator citFinded = m_connectorMap.end();

    for(ConnectorMap::const_iterator cit = m_connectorMap.begin(); cit != m_connectorMap.end(); cit++)
    {
        CNamedPipeConnector* pConnector = dynamic_cast<CNamedPipeConnector*>(*cit);

        if(NULL != pConnector && pConnector->GetHandle() == hPipe)
        {
            citFinded = cit;
            break;
        }
    }

    LeaveCriticalSection(&m_csClientMap);
    return citFinded;
}

void CNamedPipeServerImpl::Begin()
{
    EnterCriticalSection(&m_csClientMap);
    m_citCurrent = m_connectorMap.begin();
    LeaveCriticalSection(&m_csClientMap);
}

BOOL CNamedPipeServerImpl::End()
{
    BOOL bEnd = FALSE;
    EnterCriticalSection(&m_csClientMap);
    bEnd = m_citCurrent == m_connectorMap.end();
    LeaveCriticalSection(&m_csClientMap);
    return bEnd;
}

void CNamedPipeServerImpl::Next()
{
    EnterCriticalSection(&m_csClientMap);
    m_citCurrent++;
    LeaveCriticalSection(&m_csClientMap);
}

IIPCConnector* CNamedPipeServerImpl::GetCurrent()
{
    IIPCConnector* pConnector = NULL;
    EnterCriticalSection(&m_csClientMap);
    pConnector = *m_citCurrent;
    LeaveCriticalSection(&m_csClientMap);
    return pConnector;
}

void CNamedPipeServerImpl::FreeOverlapped(CNamedPipeMessage** dataOverlapped)
{
    if(NULL != *dataOverlapped)
        delete *dataOverlapped;

    *dataOverlapped = NULL;
}

DWORD WINAPI CNamedPipeServerImpl::IOCompletionThread(LPVOID lpParam)
{
    CNamedPipeServerImpl* pThis = (CNamedPipeServerImpl*)lpParam;

    if(NULL == lpParam)
        return -1;

    CIOCompletionPort* iocp = &pThis->m_iocp;
    CNamedPipeConnector* pClient = NULL;
    DWORD dwBytesTransferred = 0;
    CNamedPipeMessage* message = NULL;
    BOOL bSucess = FALSE;

    while(TRUE)
    {
        bSucess = iocp->DequeuePacket((ULONG_PTR*)&pClient, &dwBytesTransferred, (OVERLAPPED **)&message, INFINITE);

        if(!bSucess && NULL == message)
        {
            break;
        }
        else if(!bSucess && GetLastError() == ERROR_BROKEN_PIPE)
        {
            pThis->RemoveAndCloseClient(pClient);
            CNamedPipeServerImpl::FreeOverlapped(&message);
            continue;
        }

        IPC_OVERLAPPED_TYPE type = IPC_OVERLAPPED_UKNOWN;

        if(NULL != message)
            type = message->GetOvType();

        switch(type)
        {
            case IPC_OVERLAPPED_CONNECT:
                pClient->DoRead();
                pThis->WaitClientConnect();
                // the overlapped struct of the connection header doesn't need to free in here.
                // it be freed in the destruction of the class
                break;

            case IPC_OVERLAPPED_READ:
            {
                VOID* lpBuf = NULL;
                DWORD dwBufSize = 0;

                if(message->GetCustomBuffer(&lpBuf, &dwBufSize))
                    pThis->m_pEvent->OnRequest(pThis, pClient, lpBuf, dwBufSize);

                CNamedPipeServerImpl::FreeOverlapped(&message);
                pClient->DoRead();
                break;
            }

            case IPC_OVERLAPPED_WRITE:
                CNamedPipeServerImpl::FreeOverlapped(&message);
                break;

            default:
                CNamedPipeServerImpl::FreeOverlapped(&message);
                break;
        }
    }

    return 0;
}

BOOL CNamedPipeServerImpl::WaitClientConnect()
{
    CNamedPipeConnector* pClient = new CNamedPipeConnector();

    if(NULL == pClient)
        return FALSE;

    if(!pClient->Create(m_sPipeName))
    {
        delete pClient;
        pClient = NULL;
        return FALSE;
    }

    m_connectorMap.push_back(pClient);

    if(!m_iocp.AssociateDevice(pClient->GetHandle(), (ULONG_PTR)pClient))
        return FALSE;

    if(!pClient->WaitConnect())
        return FALSE;

    return TRUE;
}

void CNamedPipeServerImpl::RemoveAndCloseClient(CNamedPipeConnector* pClient)
{
    EnterCriticalSection(&m_csClientMap);
    ConnectorMap::const_iterator cit = FindClient(pClient->GetHandle());

    if(cit != m_connectorMap.end())
        m_connectorMap.erase(cit);

    pClient->Close();
    delete pClient;
    pClient = NULL;
    LeaveCriticalSection(&m_csClientMap);
}

CNamedPipeConnector::CNamedPipeConnector(): m_pConnPackage(NULL)
    , m_pLastMessage(NULL)
{
    m_pConnPackage = new CNamedPipeMessage(NULL, 0, IPC_OVERLAPPED_CONNECT);
}

CNamedPipeConnector::~CNamedPipeConnector()
{
    if(NULL != m_pLastMessage)
        delete m_pLastMessage;

    m_pLastMessage = NULL;

    if(NULL != m_pConnPackage)
        delete m_pConnPackage;

    m_pConnPackage = NULL;
}

DWORD CNamedPipeConnector::GetSID()
{
    return m_pipe.GetNamedPipeClientProcessId();
}

LPCTSTR CNamedPipeConnector::GetName()
{
    return NULL;
}

BOOL CNamedPipeConnector::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    if(NULL == lpBuf || dwBufSize <= 0)
        return FALSE;

    CNamedPipeMessage* postPackage = new CNamedPipeMessage(lpBuf, dwBufSize, IPC_OVERLAPPED_WRITE);

    DWORD dwWrited = 0;
    BOOL bSucess = m_pipe.WriteFile(&postPackage->m_package, postPackage->m_package.dwTotalSize, &dwWrited, postPackage->GetOvHeader());
    return (bSucess || GetLastError() == ERROR_IO_PENDING);
}

BOOL CNamedPipeConnector::RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize)
{
    if(NULL == lpSendBuf || dwSendBufSize <= 0)
        return FALSE;

    if(NULL == lpReplyBuf || dwReplyBufSize <= 0)
        return FALSE;

    CNamedPipeMessage* sendPackage = new CNamedPipeMessage(lpSendBuf, dwSendBufSize);

    DWORD dwWrited = 0;
    BOOL bSucess = m_pipe.WriteFile(&sendPackage->m_package, sendPackage->m_package.dwTotalSize, &dwWrited, sendPackage->GetOvHeader());

    if(!bSucess && GetLastError() == ERROR_IO_PENDING)
    {
        if(GetOverlappedResult(m_pipe.GetHandle(), sendPackage->GetOvHeader(), &dwWrited, TRUE))
            bSucess = TRUE;
    }

    CNamedPipeMessage recePackage(NULL, 0);

    DWORD dwReaded = 0;
    bSucess = m_pipe.ReadFile(&recePackage.m_package, sizeof(recePackage.m_package), &dwReaded, recePackage.GetOvHeader());

    if(!bSucess && GetLastError() == ERROR_IO_PENDING)
    {
        if(GetOverlappedResult(m_pipe.GetHandle(), recePackage.GetOvHeader(), &dwReaded, TRUE))
            bSucess = TRUE;
    }

    if(bSucess)
    {
        LPVOID* lpBuf = NULL;
        DWORD dwBufSize = 0;

        if(recePackage.GetCustomBuffer(lpBuf, &dwBufSize))
            memcpy_s(lpReplyBuf, dwReplyBufSize, lpBuf, dwBufSize);
    }

    return bSucess;
}

BOOL CNamedPipeConnector::Create(LPCTSTR lpPipeName)
{
    if(NULL == lpPipeName) return FALSE;

    return m_pipe.CreateNamedPipe(lpPipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                  PIPE_UNLIMITED_INSTANCES, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
}

void CNamedPipeConnector::Close()
{
    m_pipe.FlushFileBuffers();
    m_pipe.DisconnectNamedPipe();
    m_pipe.Close();
}

HANDLE CNamedPipeConnector::GetHandle()
{
    return m_pipe.GetHandle();
}

BOOL CNamedPipeConnector::DoRead()
{
    CNamedPipeMessage* package = new CNamedPipeMessage(NULL, 0, IPC_OVERLAPPED_READ);
    m_pLastMessage = package;
    BOOL bSucess = m_pipe.ReadFile(&package->m_package, sizeof(package->m_package), NULL, package->GetOvHeader());
    return (bSucess || ERROR_IO_PENDING == GetLastError());
}

BOOL CNamedPipeConnector::WaitConnect()
{
    BOOL bSucess = m_pipe.ConnectNamedPipe(m_pConnPackage->GetOvHeader());
    return (bSucess || GetLastError() == ERROR_IO_PENDING);
}
