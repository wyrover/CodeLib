#include "stdafx.h"
#include "LPCClientImpl.h"
#include "LPCServerImpl.h"

namespace CODELIB
{


    CLPCClientImpl::CLPCClientImpl(ILPCEvent* pEvent): m_pEvent(pEvent), m_hPort(NULL)
    {
        InitializeCriticalSection(&m_mapCS);
    }

    CLPCClientImpl::~CLPCClientImpl()
    {
        DeleteCriticalSection(&m_mapCS);
    }

    BOOL CLPCClientImpl::PostData(MESSAGE_TYPE type, LPVOID lpData, DWORD dwDataSize)
    {
        CLPCMessage sendMsg;
        sendMsg.SetBuffer(lpData, dwDataSize);
        NTSTATUS ntStatus = NtRequestPort(m_hPort, sendMsg.GetHeader());
        return NT_SUCCESS(ntStatus);
    }

    BOOL CLPCClientImpl::Create(LPCTSTR lpPortName)
    {
        HANDLE m_hConnect = NULL;
        UNICODE_STRING sPortName;
        RtlInitUnicodeString(&sPortName, lpPortName);
        ULONG uMsgSize = 0;
        ULONG uMaxInfoLength = 0;
        NTSTATUS ntStatus = NtConnectPort(&m_hConnect, &sPortName, /*&SecurityQos*/NULL,
                                          /*&ClientView*/NULL, /*&ServerView*/NULL, &uMaxInfoLength, NULL, NULL);

        if(!NT_SUCCESS(ntStatus))
            return FALSE;

        ISender* pSender = new CLPCSender(m_hConnect, 0, m_pEvent);

        if(NULL == pSender)
            return FALSE;

        AddSender(m_hConnect, pSender);

        OnConnect(this, pSender);

        return TRUE;
    }

    void CLPCClientImpl::Close()
    {
        if(NULL != m_hPort)
        {
            NtClose(m_hPort);
            m_hPort = NULL;
        }

        OnClose(this);
    }

    void CLPCClientImpl::OnCreate(ILPC* pLPC)
    {
        if(NULL != m_pEvent)
        {
            m_pEvent->OnCreate(pLPC);
        }
    }

    void CLPCClientImpl::OnClose(ILPC* pLPC)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnClose(pLPC);
    }

    BOOL CLPCClientImpl::OnConnect(ILPC* pLPC, ISender* pSender)
    {
        if(NULL != m_pEvent)
            return m_pEvent->OnConnect(pLPC, pSender);

        return FALSE;
    }

    void CLPCClientImpl::OnDisConnect(ILPC* pLPC, ISender* pSender)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnDisConnect(pLPC, pSender);
    }

    void CLPCClientImpl::OnRecv(ILPC* pLPC, ISender* pSender, IMessage* pMessage)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnRecv(pLPC, pSender, pMessage);
    }

    void CLPCClientImpl::OnRecvAndReply(ILPC* pLPC, ISender* pSender, IMessage* pReceiveMsg, IMessage* pReplyMsg)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnRecvAndReply(pLPC, pSender, pReceiveMsg, pReplyMsg);
    }

    void CLPCClientImpl::AddSender(HANDLE hPort, ISender* pSender)
    {
        EnterCriticalSection(&m_mapCS);

        if(NULL != hPort || NULL != pSender)
            m_sendersMap.insert(std::make_pair(hPort, pSender));

        LeaveCriticalSection(&m_mapCS);
    }

    void CLPCClientImpl::RemoveSender(HANDLE hPort)
    {
        EnterCriticalSection(&m_mapCS);
        m_sendersMap.erase(hPort);
        LeaveCriticalSection(&m_mapCS);
    }

    ISender* CLPCClientImpl::FindSenderByHandle(HANDLE hPort)
    {
        ISender* pSender = NULL;
        EnterCriticalSection(&m_mapCS);

        SenderMap::const_iterator cit = m_sendersMap.find(hPort);

        if(cit != m_sendersMap.end())
            pSender = cit->second;

        LeaveCriticalSection(&m_mapCS);

        return pSender;
    }

}