#include "stdafx.h"
#include "LPCServerImpl.h"
#include "ntdll.h"

namespace CODELIB
{
    CLPCServerImpl::CLPCServerImpl(ILPCEvent* pEvent): m_hListenPort(NULL), m_pEvent(pEvent)
    {
        InitializeCriticalSection(&m_mapCS);
    }

    CLPCServerImpl::~CLPCServerImpl()
    {
        Close();
        DeleteCriticalSection(&m_mapCS);
    }

    BOOL CLPCServerImpl::Create(LPCTSTR lpPortName)
    {
        // 创建端口
        SECURITY_DESCRIPTOR sd;
        OBJECT_ATTRIBUTES ObjAttr;
        UNICODE_STRING PortName;
        NTSTATUS Status;

        if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
            return FALSE;

        if(!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
            return FALSE;

        RtlInitUnicodeString(&PortName, lpPortName);
        InitializeObjectAttributes(&ObjAttr, &PortName, 0, NULL, &sd);
        Status = NtCreatePort(&m_hListenPort, &ObjAttr, NULL, sizeof(PORT_MESSAGE) + MAX_LPC_DATA, 0);

        if(!NT_SUCCESS(Status))
            return FALSE;

        // 开启监听线程监听端口连接
        m_hListenThread = CreateThread(NULL, 0, _ListenThreadProc, this, 0, NULL);

        if(NULL == m_hListenThread)
        {
            Close();
            return FALSE;
        }

        OnCreate(this);
        return TRUE;
    }

    void CLPCServerImpl::Close()
    {
        if(NULL != m_hListenPort)
        {
            NtClose(m_hListenPort);
            m_hListenPort = NULL;
        }

        if(NULL != m_hListenThread)
        {
            WaitForSingleObject(m_hListenThread, INFINITE);
            CloseHandle(m_hListenThread);
            m_hListenThread = NULL;
        }

        ClearSenders();

        OnClose(this);
    }

    void CLPCServerImpl::OnCreate(ILPC* pLPC)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnCreate(pLPC);
    }

    void CLPCServerImpl::OnClose(ILPC* pLPC)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnClose(pLPC);
    }

    BOOL CLPCServerImpl::OnConnect(ILPC* pLPC, ISender* pSender)
    {
        if(NULL != m_pEvent)
            return m_pEvent->OnConnect(pLPC, pSender);

        return TRUE;
    }

    void CLPCServerImpl::OnDisConnect(ILPC* pLPC, ISender* pSender)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnDisConnect(pLPC, pSender);
    }

    void CLPCServerImpl::OnRecv(ILPC* pLPC, ISender* pSender, IMessage* pMessage)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnRecv(pLPC, pSender, pMessage);
    }

    DWORD CLPCServerImpl::_ListenThreadProc(LPVOID lpParam)
    {
        CLPCServerImpl* pThis = (CLPCServerImpl*)lpParam;

        if(NULL == pThis)
            return -1;

        return pThis->_ListenThread();
    }

    DWORD CLPCServerImpl::_ListenThread()
    {
        NTSTATUS            Status = STATUS_UNSUCCESSFUL;
        CLPCMessage recevieMsg;
        CLPCMessage* replyMsg = NULL;
        DWORD               LpcType = 0;
        PVOID PortContext;

        for(; ;)
        {
            // 等待接收客户端消息,并回应,此函数会一直挂起,直到接收到数据
            Status = NtReplyWaitReceivePort(m_hListenPort, &PortContext, (NULL == replyMsg ? NULL : replyMsg->GetHeader()), recevieMsg.GetHeader());

            if(!NT_SUCCESS(Status))
            {
                replyMsg = NULL;
                break;
            }

            LpcType = recevieMsg.GetHeader()->u2.s2.Type;

            if(LpcType == LPC_CONNECTION_REQUEST)           // 有客户端连接到来
            {
                HandleConnect(recevieMsg.GetHeader());
                replyMsg = NULL;
                continue;
            }
            else if((LpcType == LPC_CLIENT_DIED) || (LpcType == LPC_PORT_CLOSED))             // 客户端进程退出
            {
                replyMsg = NULL;
                HandleDisConnect((HANDLE)PortContext);
                continue;
            }
            else if(LpcType == LPC_REQUEST)     // 客户端调用NtRequestWaitReplyPort时触发此消息,表示客户端发来消息,并且要求服务端进行应答
            {
                // 根据收到的消息填充应答消息的通信头结构体
                replyMsg = &recevieMsg;
                HandleRequest(HANDLE(PortContext), &recevieMsg, replyMsg);
                continue;
            }
            else if(LpcType == LPC_DATAGRAM)        // 客户端调用NtRequestPort时触发此消息,表示客户端发来消息,无需服务端进行应答
            {
                HandleDataGram(HANDLE(PortContext), &recevieMsg);
                replyMsg = NULL;
                continue;
            }
        }

        return 0;
    }

    BOOL CLPCServerImpl::HandleConnect(PPORT_MESSAGE message)
    {
        // 准备接受客户端连接
//         REMOTE_PORT_VIEW ClientView;
//         PORT_VIEW ServerView;
        HANDLE hConnect = NULL;
        NTSTATUS ntStatus = NtAcceptConnectPort(&hConnect, NULL, message, TRUE, /*&ServerView*/NULL, /*&ClientView*/NULL);

        if(!NT_SUCCESS(ntStatus))
            return FALSE;

        ntStatus = NtCompleteConnectPort(hConnect);

        if(!NT_SUCCESS(ntStatus))
            return FALSE;

        CLPCSender* pSender = new CLPCSender(hConnect, (DWORD)message->ClientId.UniqueProcess, this);

        if(NULL == pSender)
            return FALSE;

        AddSender(hConnect, pSender);

        OnConnect(this, pSender);

        return TRUE;
    }

    BOOL CLPCServerImpl::HandleDisConnect(HANDLE hPort)
    {
        CLPCSender* pSender = dynamic_cast<CLPCSender*>(FindSenderByHandle(hPort));

        if(NULL != pSender)
        {
            RemoveSender(hPort);
            pSender->DisConnect();
            OnDisConnect(this, pSender);
            delete pSender;
            pSender = NULL;
            return TRUE;
        }

        return FALSE;
    }

    HANDLE CLPCServerImpl::CreateListenThread(HANDLE hPort)
    {
        THREAD_PARAM* pParam = new THREAD_PARAM;
        pParam->pServer = this;
        pParam->hPort = hPort;
        return CreateThread(NULL, 0, _ListenThreadProc, pParam, 0, NULL);
    }

    void CLPCServerImpl::AddSender(HANDLE hPort, ISender * pSender)
    {
        EnterCriticalSection(&m_mapCS);

        if(NULL != hPort || NULL != pSender)
            m_sendersMap.insert(std::make_pair(hPort, pSender));

        LeaveCriticalSection(&m_mapCS);
    }

    void CLPCServerImpl::RemoveSender(HANDLE hPort)
    {
        EnterCriticalSection(&m_mapCS);
        m_sendersMap.erase(hPort);
        LeaveCriticalSection(&m_mapCS);
    }

    ISender* CLPCServerImpl::FindSenderByHandle(HANDLE hPort)
    {
        ISender* pSender = NULL;
        EnterCriticalSection(&m_mapCS);

        SenderMap::const_iterator cit = m_sendersMap.find(hPort);

        if(cit != m_sendersMap.end())
            pSender = cit->second;

        LeaveCriticalSection(&m_mapCS);

        return pSender;
    }

    void CLPCServerImpl::ClearSenders()
    {
        SenderMap::const_iterator cit;

        for(cit = m_sendersMap.begin(); cit != m_sendersMap.end(); cit++)
        {
            CLPCSender* pSender = dynamic_cast<CLPCSender*>(cit->second);

            if(NULL != pSender)
            {
                pSender->DisConnect();
                delete pSender;
                pSender = NULL;
            }
        }

        m_sendersMap.clear();
    }

    BOOL CLPCServerImpl::HandleDataGram(HANDLE hPort, IMessage* message)
    {
        CLPCSender* pSender = dynamic_cast<CLPCSender*>(FindSenderByHandle(hPort));

        if(NULL != pSender)
            OnRecv(this, pSender, message);

        return TRUE;
    }

    BOOL CLPCServerImpl::HandleRequest(HANDLE hPort, IMessage* receiveMsg, IMessage* replyMsg)
    {
        CLPCSender* pSender = dynamic_cast<CLPCSender*>(FindSenderByHandle(hPort));

        if(NULL != pSender)
            OnRecvAndReply(this, pSender, receiveMsg, replyMsg);

        return TRUE;
    }

    void CLPCServerImpl::OnRecvAndReply(ILPC* pLPC, ISender* pSender, IMessage* pReceiveMsg, IMessage* pReplyMsg)
    {
        if(NULL != m_pEvent)
            m_pEvent->OnRecvAndReply(pLPC, pSender, pReceiveMsg, pReplyMsg);
    }

    //////////////////////////////////////////////////////////////////////////
    DWORD CLPCSender::GetSID()
    {
        return m_dwPID;
    }

    CLPCSender::CLPCSender(HANDLE hPort, DWORD dwPID, ILPCEvent * pEvent): m_hPort(hPort)
        , m_dwPID(dwPID)
        , m_pEvent(pEvent)
    {

    }

    CLPCSender::~CLPCSender()
    {

    }

    void CLPCSender::DisConnect()
    {
        if(NULL != m_hPort)
        {
            NtClose(m_hPort);
            m_hPort = NULL;
        }
    }

    BOOL CLPCSender::SendMessage(IMessage* pMessage)
    {
        return FALSE;
    }

    BOOL CLPCSender::PostMessage(IMessage* pMessage)
    {
        if(NULL == m_hPort)
            return FALSE;

        NTSTATUS ntStatus = NtReplyPort(m_hPort, pMessage->GetHeader());
        return NT_SUCCESS(ntStatus);
    }

    //////////////////////////////////////////////////////////////////////////
    CLPCMessage::CLPCMessage(): m_dwBufSize(0)
    {
        InitializeMessageHeader(&m_portHeader, sizeof(CLPCMessage), 0);
        ZeroMemory(m_lpFixBuf, MAX_LPC_DATA);
    }

    CLPCMessage::~CLPCMessage()
    {

    }

    MESSAGE_TYPE CLPCMessage::GetMessageType()
    {
        return m_msgType;
    }

    LPVOID CLPCMessage::GetBuffer(DWORD & dwBufferSize)
    {
        LPVOID lpBuf = m_lpFixBuf;
        dwBufferSize = m_dwBufSize;
        return lpBuf;
    }

    void CLPCMessage::SetMessageType(MESSAGE_TYPE msgType)
    {
        m_msgType = msgType;
    }

    void CLPCMessage::SetBuffer(LPVOID lpBuf, DWORD dwBufSize)
    {
        if((NULL != lpBuf) && (0 != dwBufSize))
        {
            memcpy_s(m_lpFixBuf, MAX_LPC_DATA, lpBuf, dwBufSize);
            m_dwBufSize = dwBufSize;
        }
    }

    PPORT_MESSAGE CLPCMessage::GetHeader()
    {
        return &m_portHeader;
    }

}