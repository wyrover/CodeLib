/********************************************************************
    created:    2014/04/26
    file base:  IIPCInterface
    author:     redcode
    purpose:    the interface for the IPC
*********************************************************************/

#pragma once
#include <windows.h>
#include <vector>

#define pure_virtual __declspec(novtable)

struct pure_virtual IIPCMessage
{
    virtual ~IIPCMessage() = 0 {};
    virtual DWORD GetID() = 0;
    virtual BOOL GetBuffer(LPVOID lpBuf, LPDWORD dwBufferSize) = 0;
};

struct pure_virtual IIPCConnector
{
    virtual ~IIPCConnector() = 0 {};
    virtual DWORD GetSID() = 0;
    virtual LPCTSTR GetName() = 0;
    virtual BOOL PostMessage(LPCVOID lpBuf, DWORD dwBufSize) = 0;
    virtual BOOL RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, DWORD dwTimeout = 3000) = 0;
};

struct pure_virtual IIPCConnectorIterator
{
    virtual ~IIPCConnectorIterator() = 0 {};
    virtual void Begin() = 0;
    virtual BOOL End() = 0;
    virtual void Next() = 0;
    virtual IIPCConnector* GetCurrent() = 0;
};

typedef std::vector<IIPCConnector*> ConnectorMap;

struct pure_virtual IIPCObject
{
    virtual ~IIPCObject() = 0 {};
    virtual BOOL Create(LPCTSTR lpPipeName) = 0;
    virtual void Close() = 0;
    virtual IIPCConnectorIterator* GetClients() = 0;
};

struct pure_virtual IIPCEvent
{
    virtual ~IIPCEvent() = 0 {};
    virtual void OnRequest(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize) = 0;
};