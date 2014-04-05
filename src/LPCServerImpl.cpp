#include "stdafx.h"
#include "LPCServerImpl.h"
#include "ntdll.h"

namespace CODELIB
{


    CLPCServerImpl::CLPCServerImpl(ILPCEvent* pEvent): m_hListenPort(NULL), m_pEvent(pEvent)
    {

    }

    CLPCServerImpl::~CLPCServerImpl()
    {
        Close();
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
        m_hListenThread = CreateListenThread(m_hListenPort);

        if(NULL == m_hListenThread)
        {
            Close();
            return FALSE;
        }

//         CLPCSender* pSender = new CLPCSender(m_hListenPort, this);
//
//         if(NULL == pSender)
//             return FALSE;
//
//         AddSender(m_hListenPort, pSender);
//         pSender->Connect();

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

        OnClose(this);
    }

    ISenders* CLPCServerImpl::GetSenders()
    {
        static CLPCSenders senders(m_sendersMap);
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
        TRANSFERRED_DATA    ReceiveData;            // 接收请求数据
        PTRANSFERRED_DATA   ReplyData = NULL;       // 回送数据
        DWORD               LpcType = 0;

        for(; ;)
        {
            // 等待接收客户端消息,并回应,此函数会一直挂起,直到接收到数据
            Status = NtReplyWaitReceivePort(hPort, 0, &ReplyData->Header, &ReceiveData.Header);

            if(!NT_SUCCESS(Status))
            {
                ReplyData = NULL;
                break;
            }

            LpcType = ReceiveData.Header.u2.s2.Type;

            // 有客户端连接到来
            if(LpcType == LPC_CONNECTION_REQUEST)
            {
                HandleConnect(&ReceiveData.Header);
                ReplyData = NULL;
                continue;
            }

            // 客户端进程退出
            if(LpcType == LPC_CLIENT_DIED)
            {
                ReplyData = NULL;

                if(!HandleDisConnect(hPort))
                    break;

                continue;
            }

            // 客户端端口关闭
            if(LpcType == LPC_PORT_CLOSED)
            {
                ReplyData = NULL;

//                 if(!HandleDisConnect(hPort))
//                     break;
//
//                 continue;
                break;
            }

            // 客户端调用NtRequestWaitReplyPort时触发此消息,表示客户端发来消息,并且要求服务端进行应答
            if(LpcType == LPC_REQUEST)
            {
                // 根据收到的消息填充应答消息的通信头结构体
                ReplyData = &ReceiveData;
//              HandleRequest(hPort, &ReceiveData, ReplyData);
                continue;
            }

            // 客户端调用NtRequestPort时触发此消息,表示客户端发来消息,无需服务端进行应答
            if(LpcType == LPC_DATAGRAM)
            {
                ReplyData = NULL;
//              HandleRequest(hPort, &ReceiveData, ReplyData);
                continue;
            }
        }

        return 0;
    }

    BOOL CLPCServerImpl::HandleConnect(PPORT_MESSAGE message)
    {
        // 准备接受客户端连接
        REMOTE_PORT_VIEW ClientView;
        PORT_VIEW ServerView;
        HANDLE hConnect = NULL;
        NTSTATUS ntStatus = NtAcceptConnectPort(&hConnect, NULL, message, TRUE, /*&ServerView*/NULL, /*&ClientView*/NULL);

        if(!NT_SUCCESS(ntStatus))
            return FALSE;

        ntStatus = NtCompleteConnectPort(hConnect);

        if(!NT_SUCCESS(ntStatus))
            return FALSE;

        CLPCSender* pSender = new CLPCSender(m_hListenPort, this);

        if(NULL == pSender)
            return FALSE;

        AddSender(hConnect, pSender);

        return pSender->Connect();
    }

    BOOL CLPCServerImpl::HandleDisConnect(HANDLE hPort)
    {
        // 一定要判断是从服务端口退出还是从客户端口退出
        if(m_hListenPort == hPort)  // 从服务端口退出
        {
            // 断开所有客户端
//            ClearSenders();
            return TRUE;
        }
        else
        {
            CLPCSender* pSender = (CLPCSender*)FindSenderByHandle(hPort);

            if(NULL != pSender)
            {
                RemoveSender(hPort);
                pSender->DisConnect(TRUE);
                delete pSender;
                pSender = NULL;
                return FALSE;
            }
        }

        return FALSE;
    }

    HANDLE CLPCServerImpl::CreateListenThread(HANDLE hPort)
    {
        // 开启监听线程监听端口连接
        THREAD_PARAM* pParam = new THREAD_PARAM;
        pParam->pServer = this;
        pParam->hPort = hPort;
        return CreateThread(NULL, 0, _ListenThreadProc, pParam, 0, NULL);
    }

    void CLPCServerImpl::AddSender(HANDLE hPort, ISender* pSender)
    {
        if(NULL != hPort || NULL != pSender)
            m_sendersMap.insert(std::make_pair(hPort, pSender));
    }

    void CLPCServerImpl::RemoveSender(HANDLE hPort)
    {
        SenderMap::const_iterator cit;
        SenderMap::const_iterator citRemove = m_sendersMap.end();

        for(cit = m_sendersMap.begin(); cit != m_sendersMap.end(); cit++)
        {
            CLPCSender* pSender = dynamic_cast<CLPCSender*>(cit->second);

            if((NULL != pSender) && (pSender->GetHandle() == hPort))
            {
                citRemove = cit;
                break;
            }
        }

        if(citRemove != m_sendersMap.end())
            m_sendersMap.erase(citRemove);
    }

    ISender* CLPCServerImpl::FindSenderByHandle(HANDLE hPort)
    {
        ISenders* pSenders = GetSenders();

        for(pSenders->Begin(); !pSenders->End(); pSenders->Next())
        {
            CLPCSender* pSender = dynamic_cast<CLPCSender*>(pSenders->GetCurrent());

            if((NULL != pSender) && (pSender->GetHandle() == hPort))
                return pSender;
        }

        return NULL;
    }

    HANDLE CLPCServerImpl::GetListenPort()
    {
        return m_hListenPort;
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

    //////////////////////////////////////////////////////////////////////////
    CLPCSenders::CLPCSenders(SenderMap senderMap): m_senderMap(senderMap)
    {
        m_cit = m_senderMap.begin();
    }

    CLPCSenders::~CLPCSenders()
    {
        m_cit = m_senderMap.end();
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
        m_cit++;
    }

    ISender* CLPCSenders::GetCurrent()
    {
        if(End())
            return NULL;

        return (ISender*)m_cit->second;
    }

    DWORD CLPCSenders::GetSize()
    {
        return m_senderMap.size();
    }

    //////////////////////////////////////////////////////////////////////////
    DWORD CLPCSender::GetSID()
    {
        return 0;
    }

    IMessage* CLPCSender::AllocMessage()
    {
        return NULL;
    }

    void CLPCSender::FreeMessage(IMessage* pMessage)
    {

    }

    BOOL CLPCSender::SendMessage(IMessage* pMessage)
    {
        return FALSE;
    }

    CLPCSender::CLPCSender(HANDLE hPort, CLPCServerImpl* pServer): m_hPort(hPort)
        , m_pServer(pServer)
        , m_hListenThread(NULL)
    {

    }

    CLPCSender::~CLPCSender()
    {

    }

    BOOL CLPCSender::Connect()
    {
        if(NULL == m_pServer)
            return FALSE;

        if(NULL == m_hListenThread)
            m_hListenThread = m_pServer->CreateListenThread(m_hPort);

        m_pServer->OnConnect(m_pServer, this);

//      WaitForSingleObject(m_hListenThread,INFINITE);
        return (NULL != m_hListenThread);
    }

    void CLPCSender::DisConnect(BOOL bSelfExit)
    {
        if(NULL != m_hListenThread)
        {
            if(!bSelfExit)  // 从服务端口退出
            {
                // 此时,如果接收线程正在等待消息的话,会处于挂起状态,无法结束,所以可以强制结束线程
                if(WAIT_OBJECT_0 != WaitForSingleObject(m_hListenThread, 100))
                    TerminateThread(m_hListenThread, 0);
            }

            CloseHandle(m_hListenThread);
            m_hListenThread = NULL;
        }

        if(NULL != m_hPort)
        {
            NtClose(m_hPort);
            m_hPort = NULL;
        }

        if(NULL != m_pServer)
            m_pServer->OnDisConnect(m_pServer, this);
    }

    HANDLE CLPCSender::GetHandle()
    {
        return m_hPort;
    }

}