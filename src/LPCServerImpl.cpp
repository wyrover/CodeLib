#include "stdafx.h"
#include "LPCServerImpl.h"
#include "ntdll.h"

namespace CODELIB
{
    CLPCServerImpl::CLPCServerImpl(ILPCEvent* pEvent): m_pEvent(pEvent), m_hListenPort(NULL), m_hListenThread(NULL)
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

        m_hListenThread = CreateListenThread(m_hListenPort);

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
        ClearSenders();

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

        OnClose(this);
    }

    ISenders* CLPCServerImpl::GetSenders()
    {
        EnterCriticalSection(&m_mapCS);
        static CLPCSenders senders(m_sendersMap);
        LeaveCriticalSection(&m_mapCS);
        return &senders;
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
        THREAD_PARAM* threadParam = (THREAD_PARAM*)lpParam;

        if(NULL == threadParam)
            return -1;

        CLPCServerImpl* pThis = (CLPCServerImpl*)threadParam->pServer;
        HANDLE hPort = threadParam->hPort;
        DWORD dwRet = pThis->_ListenThread(hPort);

        delete threadParam;
        threadParam = NULL;
        return dwRet;
    }

    DWORD CLPCServerImpl::_ListenThread(HANDLE hPort)
    {
        NTSTATUS            Status = STATUS_UNSUCCESSFUL;

        CLPCMessage    ReceiveData;            // 接收请求数据
        CLPCMessage*   ReplyData = NULL;       // 回送数据
        DWORD               LpcType = 0;

        for(; ;)
        {
            // 等待接收客户端消息,并回应,此函数会一直挂起,直到接收到数据
            Status = NtReplyWaitReceivePort(m_hListenPort, 0, NULL == ReplyData ? NULL : ReplyData->GetHeader(), ReceiveData.GetHeader());

            if(!NT_SUCCESS(Status))
            {
                ReplyData = NULL;
                break;
            }

            LpcType = ReceiveData.GetHeader()->u2.s2.Type;

            if(LpcType == LPC_CONNECTION_REQUEST)           // 有客户端连接到来
            {
                HandleConnect(&ReceiveData);
                ReplyData = NULL;
                continue;
            }
            else if(LpcType == LPC_CLIENT_DIED)             // 客户端进程退出
            {
                ReplyData = NULL;
                HandleDisConnect(hPort);
                continue;
            }
            else if(LpcType == LPC_PORT_CLOSED)             // 客户端端口关闭
            {
                ReplyData = NULL;
                HandleDisConnect(hPort);
                break;

            }
            else if(LpcType == LPC_REQUEST)     // 客户端调用NtRequestWaitReplyPort时触发此消息,表示客户端发来消息,并且要求服务端进行应答
            {
                // 根据收到的消息填充应答消息的通信头结构体
                CLPCMessage replyMsg;
                replyMsg.SetHeader(*(ReceiveData.GetHeader()));
                ReplyData = &replyMsg;
                HandleRequest(hPort, &ReceiveData, ReplyData);
                continue;
            }
            else if(LpcType == LPC_DATAGRAM)        // 客户端调用NtRequestPort时触发此消息,表示客户端发来消息,无需服务端进行应答
            {
                HandleRequest(hPort, &ReceiveData, NULL);
                continue;
            }
            else
            {
                ReplyData = NULL;
                continue;
            }
        }

        return 0;
    }

    BOOL CLPCServerImpl::HandleConnect(CLPCMessage* connectInfo)
    {
        // 准备接受客户端连接
//         REMOTE_PORT_VIEW ClientView;
//         PORT_VIEW ServerView;

        HANDLE hConnect = NULL;
        NTSTATUS ntStatus = NtAcceptConnectPort(&hConnect, NULL, connectInfo->GetHeader(), TRUE, /*&ServerView*/NULL, /*&ClientView*/NULL);

        if(!NT_SUCCESS(ntStatus))
            return FALSE;

        ntStatus = NtCompleteConnectPort(hConnect);

        if(!NT_SUCCESS(ntStatus))
            return FALSE;

        CLPCSender* pSender = AddSender(hConnect);

        if(NULL == pSender)
            return FALSE;

        return OnConnect(this, pSender);
    }

    BOOL CLPCServerImpl::HandleDisConnect(HANDLE hPort)
    {
        CLPCSender* pSender = dynamic_cast<CLPCSender*>(FindSenderByHandle(hPort));

        if(NULL != pSender)
        {
            RemoveSender(hPort);

            pSender->DisConnect(this);
            OnDisConnect(this, pSender);

            delete pSender;
            pSender = NULL;
        }

        return FALSE;
    }

    BOOL CLPCServerImpl::HandleRequest(HANDLE hPort, CLPCMessage* recevieData, CLPCMessage* replyData)
    {
        CLPCSender* pSender = dynamic_cast<CLPCSender*>(FindSenderByHandle(hPort));

        if(NULL != pSender)
        {
            OnRecv(this, pSender, recevieData);
        }

        return TRUE;
    }

    HANDLE CLPCServerImpl::CreateListenThread(HANDLE hPort)
    {
        THREAD_PARAM* pParam = new THREAD_PARAM;
        pParam->pServer = this;
        pParam->hPort = hPort;
        return CreateThread(NULL, 0, _ListenThreadProc, pParam, 0, NULL);
    }

    void CLPCServerImpl::AddSender(HANDLE hPort, ISender* pSender)
    {
        EnterCriticalSection(&m_mapCS);

        if(NULL != hPort || NULL != pSender)
            m_sendersMap.insert(std::make_pair(hPort, pSender));

        LeaveCriticalSection(&m_mapCS);
    }

    CLPCSender* CLPCServerImpl::AddSender(HANDLE hPort)
    {
        CLPCSender* pSender = new CLPCSender(hPort);

        if(NULL == pSender)
            return FALSE;

        if(!pSender->Connect(this))
        {
            delete pSender;
            pSender = NULL;
            return NULL;
        }

        AddSender(hPort, pSender);

        return pSender;
    }

    void CLPCServerImpl::RemoveSender(HANDLE hPort)
    {
        EnterCriticalSection(&m_mapCS);
        m_sendersMap.erase(hPort);
        LeaveCriticalSection(&m_mapCS);
    }

    ISender* CLPCServerImpl::FindSenderByHandle(HANDLE hPort)
    {
        EnterCriticalSection(&m_mapCS);
        ISender* pSender = NULL;
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
                pSender->DisConnect(this);
                delete pSender;
                pSender = NULL;
            }
        }

        m_sendersMap.clear();
    }

    HANDLE CLPCServerImpl::GetListenPortHandle()
    {
        return m_hListenPort;
    }


    //////////////////////////////////////////////////////////////////////////
    CLPCSenders::CLPCSenders(SenderMap senderMap): m_senderMap(senderMap)
    {
        m_cit = m_senderMap.begin();
    }

    CLPCSenders::~CLPCSenders()
    {
//        m_cit = m_senderMap.end();
    }

    void CLPCSenders::Begin()
    {
        m_cit = m_senderMap.begin();
    }

    BOOL CLPCSenders::End()
    {
        return (m_cit == m_senderMap.end());
    }

    void CLPCSenders::Next()
    {
        if(!End())
            m_cit++;
    }

    ISender* CLPCSenders::GetCurrent()
    {
        if(!End())
            return m_cit->second;

        return NULL;
    }

    DWORD CLPCSenders::GetSize()
    {
        return (DWORD)m_senderMap.size();
    }

    //////////////////////////////////////////////////////////////////////////
    DWORD CLPCSender::GetSID()
    {
        return 0;
    }

    BOOL CLPCSender::SendMessage(IMessage* pMessage)
    {
        return FALSE;
    }

    CLPCSender::CLPCSender(HANDLE hPort): m_hPort(hPort)
        , m_hListenThread(NULL)
    {

    }

    CLPCSender::~CLPCSender()
    {

    }

    BOOL CLPCSender::Connect(CLPCServerImpl* pServer)
    {
        if(NULL == pServer)
            return FALSE;

        if(NULL == m_hListenThread)
            m_hListenThread = pServer->CreateListenThread(m_hPort);

        return (NULL != m_hListenThread);
    }

    void CLPCSender::DisConnect(CLPCServerImpl* pServer)
    {
        if(NULL != m_hPort && (m_hPort != pServer->GetListenPortHandle()))
        {
            NtClose(m_hPort);
            m_hPort = NULL;
        }

        if(NULL != m_hListenThread)
        {
            CloseHandle(m_hListenThread);
            m_hListenThread = NULL;
        }
    }

    BOOL CLPCSender::AllocMessage(IMessage* pMessage)
    {

        return FALSE;
    }

    void CLPCSender::FreeMessage(IMessage* pMessage)
    {

    }

    //////////////////////////////////////////////////////////////////////////
    CLPCMessage::CLPCMessage(): m_dwBufSize(0)
        , m_bUseSectionView(FALSE)
    {
        InitializeMessageHeader(&m_LpcHeader, sizeof(CLPCMessage), 0);
        ZeroMemory(m_lpFixedBuf, FIXEDBUFLEN);
    }

    CLPCMessage::~CLPCMessage()
    {

    }

    CODELIB::MESSAGE_TYPE CLPCMessage::GetMessageType()
    {
        return m_messageType;
    }

    LPVOID CLPCMessage::GetBuffer(DWORD& dwBufferSize)
    {
        LPVOID lpBuf = NULL;

        if(IsUseSectionView())
        {

        }
        else
        {
            lpBuf = m_lpFixedBuf;
            dwBufferSize = m_dwBufSize;
        }

        return lpBuf;
    }

    void CLPCMessage::SetMessageType(MESSAGE_TYPE messageType)
    {
        m_messageType = messageType;
    }

    void CLPCMessage::SetBuffer(LPVOID lpBuf, DWORD dwBufSize)
    {
        if(dwBufSize <= FIXEDBUFLEN)    // 数据大小不大于LPC内定长度,则直接使用
        {
            m_bUseSectionView = FALSE;
            memcpy_s(m_lpFixedBuf, FIXEDBUFLEN, lpBuf, dwBufSize);
            m_dwBufSize = dwBufSize;
        }
        else // 否则的话要使用内存映射文件
        {
            m_bUseSectionView = TRUE;
        }
    }

    BOOL CLPCMessage::IsUseSectionView()
    {
        return m_bUseSectionView;
    }

    PPORT_MESSAGE CLPCMessage::GetHeader()
    {
        return &m_LpcHeader;
    }

    void CLPCMessage::SetHeader(PORT_MESSAGE lpcHeader)
    {
        memcpy_s(&m_LpcHeader, sizeof(PORT_MESSAGE), &lpcHeader, sizeof(PORT_MESSAGE));
    }

}