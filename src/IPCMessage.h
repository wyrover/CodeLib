/********************************************************************
	created:	2014/04/26
	file base:	IPCMessage 
	author:		redcode
	purpose:	the implement for the NamedPipe Message struct
*********************************************************************/

#pragma once
#include "IIPCInterface.h"

enum IPC_OVERLAPPED_TYPE
{
    IPC_OVERLAPPED_UKNOWN,
    IPC_OVERLAPPED_CONNECT,
    IPC_OVERLAPPED_READ,
    IPC_OVERLAPPED_WRITE,
};

const int FIXED_MAXIMUM_MESSAGE = 4096;

typedef struct _IPC_OVERLAPPED_HEADER : OVERLAPPED
{
    IPC_OVERLAPPED_TYPE ovType;
} IPC_OVERLAPPED_HEADER, *LPIPC_OVERALPPED_HEADER;

typedef struct _IPC_DATA_PACKAGE
{
    DWORD dwPackageID;                                          // package id
    DWORD dwUserDataSize;                                       // custom data size
    BYTE lpUserData[FIXED_MAXIMUM_MESSAGE];                    // custom buf
    DWORD dwTotalSize;                                          // the total size of the package
    FILETIME ftOccurance;                                       // the time of the package been used

} IPC_DATA_PACKAGE, *LPIPC_DATA_PACKAGE;

struct CNamedPipeMessage
{
    friend class CNamedPipeConnector;
	friend class CNamedPipeClientImpl;
private:
    IPC_OVERLAPPED_HEADER m_ovHeader;       // overlapped header
    IPC_DATA_PACKAGE m_package;             // the communication package
public:
    CNamedPipeMessage(LPCVOID lpData, DWORD dwDataSize , IPC_OVERLAPPED_TYPE ovType = IPC_OVERLAPPED_UKNOWN, DWORD dwPackageID = 0)
    {
        // init overlapped header
        ZeroMemory(this, sizeof(CNamedPipeMessage));
        m_ovHeader.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_ovHeader.ovType = ovType;

        // init package
        m_package.dwPackageID = dwPackageID;

        if(NULL != lpData && dwDataSize <= FIXED_MAXIMUM_MESSAGE)
        {
            memcpy_s(m_package.lpUserData, FIXED_MAXIMUM_MESSAGE, lpData, dwDataSize);
            m_package.dwUserDataSize = dwDataSize;
        }

        m_package.dwTotalSize = sizeof(IPC_DATA_PACKAGE) - FIXED_MAXIMUM_MESSAGE + dwDataSize;
        GetSystemTimeAsFileTime(&m_package.ftOccurance);
    }

    ~CNamedPipeMessage()
    {
        if(NULL != m_ovHeader.hEvent)
        {
            CloseHandle(m_ovHeader.hEvent);
            m_ovHeader.hEvent = NULL;
        }

        ZeroMemory(this, sizeof(CNamedPipeMessage));
    }

    LPOVERLAPPED GetOvHeader()
    {
        return &m_ovHeader;
    }

    IPC_OVERLAPPED_TYPE GetOvType()
    {
        return m_ovHeader.ovType;
    }

    DWORD GetID()
    {
        return m_package.dwPackageID;
    }

    BOOL GetCustomBuffer(LPVOID* lpBuf, LPDWORD dwBufferSize)
    {
        assert(NULL != lpBuf && NULL == *lpBuf);

        if(m_package.dwUserDataSize > 0)
        {
            *lpBuf = m_package.lpUserData;

            if(NULL != dwBufferSize)
                *dwBufferSize = m_package.dwUserDataSize;

            return TRUE;
        }

        return FALSE;
    }
};
