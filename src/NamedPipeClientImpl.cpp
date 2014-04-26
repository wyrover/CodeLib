#include "stdafx.h"
#include "NamedPipeClientImpl.h"

CNamedPipeClientImpl::CNamedPipeClientImpl(IIPCEvent* pEvent): m_pEvent(pEvent)
    , m_hThreadIOCP(NULL)
    , m_dwProcessID(0)
    , m_dwIOCPThreadNum(0)
{
    SYSTEM_INFO sysInfo = { 0 };
    GetNativeSystemInfo(&sysInfo);
    m_dwIOCPThreadNum = sysInfo.dwNumberOfProcessors * 2;
}


CNamedPipeClientImpl::~CNamedPipeClientImpl(void)
{
    Close();
}

BOOL CNamedPipeClientImpl::Create(LPCTSTR lpPipeName)
{
    TCHAR sPipeName[MAX_PATH] = {0};
    _tcscpy_s(sPipeName, MAX_PATH, _T("\\\\.\\pipe\\"));
    _tcscat_s(sPipeName, lpPipeName);

    while(TRUE)
    {
        if(m_pipe.CreateFile(sPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL))
            break;

        DWORD dwError = GetLastError();

        if(ERROR_PIPE_BUSY != dwError)
            return FALSE;

        if(!m_pipe.WaitNamedPipe(sPipeName, 5000))
            return FALSE;
    }

    if(!m_iocp.Create(m_dwIOCPThreadNum))
        return FALSE;

    m_hThreadIOCP = new HANDLE[m_dwIOCPThreadNum];

    for(DWORD i = 0; i < m_dwIOCPThreadNum; ++i)
        m_hThreadIOCP[i] = CreateThread(0, 0, IOCompletionThread, this, 0, NULL);

    DWORD dwMode = PIPE_READMODE_MESSAGE ;

    if(!m_pipe.SetNamedPipeHandleState(&dwMode, NULL, NULL))
        return FALSE;

    if(!m_iocp.AssociateDevice(m_pipe.GetHandle(), (ULONG_PTR)this))
        return FALSE;

    return TRUE;
}

void CNamedPipeClientImpl::Close()
{
    m_pipe.Close();
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
}

HANDLE CNamedPipeClientImpl::GetHandle()
{
    return m_pipe.GetHandle();
}

BOOL CNamedPipeClientImpl::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    if(NULL == lpBuf || dwBufSize <= 0)
        return FALSE;

    CNamedPipeMessage* postPackage = new CNamedPipeMessage(lpBuf, dwBufSize, IPC_OVERLAPPED_WRITE);

    DWORD dwWrited = 0;
    BOOL bSucess = m_pipe.WriteFile(&postPackage->m_package, postPackage->m_package.dwTotalSize, &dwWrited, postPackage->GetOvHeader());
    return (bSucess || GetLastError() == ERROR_IO_PENDING);
}

IIPCConnectorIterator* CNamedPipeClientImpl::GetClients()
{
    return this;
}

void CNamedPipeClientImpl::Begin()
{
    m_iIterator = 0;
}

BOOL CNamedPipeClientImpl::End()
{
    return (1 == m_iIterator);
}

void CNamedPipeClientImpl::Next()
{
    m_iIterator++;
}

IIPCConnector* CNamedPipeClientImpl::GetCurrent()
{
    return this;
}

BOOL CNamedPipeClientImpl::RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize)
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

    CNamedPipeMessage* recePackage = new CNamedPipeMessage(NULL, 0);

    DWORD dwReaded = 0;
    bSucess = m_pipe.ReadFile(&recePackage->m_package, sizeof(recePackage->m_package), &dwReaded, recePackage->GetOvHeader());

    if(!bSucess && GetLastError() == ERROR_IO_PENDING)
    {
        if(GetOverlappedResult(m_pipe.GetHandle(), recePackage->GetOvHeader(), &dwReaded, TRUE))
            bSucess = TRUE;
    }

    if(bSucess)
    {
        VOID* lpBuf = NULL;
        DWORD dwBufSize = 0;

        if(recePackage->GetCustomBuffer(&lpBuf, &dwBufSize))
            memcpy_s(lpReplyBuf, dwReplyBufSize, lpBuf, dwBufSize);
    }

    return bSucess;
}


DWORD CNamedPipeClientImpl::GetSID()
{
    return m_dwProcessID;
}

LPCTSTR CNamedPipeClientImpl::GetName()
{
    return m_sName;
}

DWORD CNamedPipeClientImpl::IOCompletionThread(LPVOID lpParam)
{
    CNamedPipeClientImpl* pThis = (CNamedPipeClientImpl*)lpParam;

    if(NULL == lpParam)
        return -1;

    CIOCompletionPort* iocp = &pThis->m_iocp;
    CNamedPipeClientImpl* pClient = NULL;
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
            CNamedPipeClientImpl::FreeOverlapped(message);
            continue;
        }

        IPC_OVERLAPPED_TYPE type = IPC_OVERLAPPED_UKNOWN;

        if(NULL != message)
            type = message->GetOvType();

        switch(type)
        {
            case IPC_OVERLAPPED_READ:
            {
                VOID* lpBuf = NULL;
                DWORD dwBufSize = 0;

                if(message->GetCustomBuffer(&lpBuf, &dwBufSize))
                    pThis->m_pEvent->OnRequest(pThis, pClient, lpBuf, dwBufSize);

                CNamedPipeClientImpl::FreeOverlapped(message);
                pClient->DoRead();
                break;
            }

            case IPC_OVERLAPPED_WRITE:
                CNamedPipeClientImpl::FreeOverlapped(message);
                break;

            default:
                CNamedPipeClientImpl::FreeOverlapped(message);
                break;
        }
    }

    _tprintf(_T("DequeuePacket failed w/err 0x%08lx\n"), GetLastError());

    return 0;
}

BOOL CNamedPipeClientImpl::CloseConnection(CNamedPipeClientImpl* pConnector)
{
    m_pipe.FlushFileBuffers();
    m_pipe.DisconnectNamedPipe();
    m_pipe.Close();
    return TRUE;
}

BOOL CNamedPipeClientImpl::DoRead()
{
    CNamedPipeMessage* package = new CNamedPipeMessage(NULL, 0, IPC_OVERLAPPED_READ);
    BOOL bSucess = m_pipe.ReadFile(&package->m_package, sizeof(package->m_package), NULL, package->GetOvHeader());
    return (bSucess || ERROR_IO_PENDING == GetLastError());
}

void CNamedPipeClientImpl::FreeOverlapped(CNamedPipeMessage* dataOverlapped)
{
    if(NULL != dataOverlapped)
        delete dataOverlapped;

    dataOverlapped = NULL;
}
