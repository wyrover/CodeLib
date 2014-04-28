/********************************************************************
    created:    2014/04/26
    file base:  NamedPipeServerImpl
    author:     redcode
    purpose:    the implement for the NamedPipe Server
*********************************************************************/

#pragma once
#include "IIPCInterface.h"
#include "IoCompletePort.h"
#include "NamedPipeWrapper.h"
#include "IPCMessage.h"

class CNamedPipeServerImpl: public IIPCObject, public IIPCConnectorIterator
{
public:
    CNamedPipeServerImpl(IIPCEvent* pEvent);

    virtual ~CNamedPipeServerImpl();

    virtual BOOL Create(LPCTSTR lpPipeName);

    virtual void Close();

    virtual IIPCConnectorIterator* GetClients();

    virtual void Begin();

    virtual BOOL End();

    virtual void Next();

    virtual IIPCConnector* GetCurrent();

    static void FreeOverlapped(CNamedPipeMessage* dataOverlapped);

protected:

    static DWORD WINAPI IOCompletionThread(LPVOID lpParam);

    BOOL WaitClientConnect();

    ConnectorMap::const_iterator FindClient(HANDLE hPipe);

    void RemoveAndCloseClient(CNamedPipeConnector* pClient);

private:

    CRITICAL_SECTION m_csClientMap;

    CIOCompletionPort m_iocp;

    ConnectorMap m_connectorMap;

    ConnectorMap::const_iterator m_citCurrent;

    TCHAR m_sPipeName[MAX_PATH];

    HANDLE* m_hThreadIOCP;

    IIPCEvent* m_pEvent;

    DWORD m_dwIOCPThreadNum;
};

class CNamedPipeConnector : public IIPCConnector
{
public:
    CNamedPipeConnector();

    virtual ~CNamedPipeConnector();

    BOOL Create(LPCTSTR lpPipeName);

    BOOL WaitConnect();

    void Close();

    BOOL DoRead();

    HANDLE GetHandle();

    virtual DWORD GetSID();

    virtual LPCTSTR GetName();

    virtual BOOL PostMessage(LPCVOID lpBuf, DWORD dwBufSize);

    virtual BOOL RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplySize, DWORD dwTimeout = 3000);

    friend class CNamedPipeServerImpl;

private:
    CNamedPipeWrapper m_pipe;

    DWORD m_dwProcessID;

    CNamedPipeMessage* m_pConnPackage;
};