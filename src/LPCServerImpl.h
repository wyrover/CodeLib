#pragma once
#include "Common.h"
#include <map>
#include "ILPC.h"

namespace CODELIB
{
    typedef std::map<HANDLE, ISender*> SenderMap;

    struct THREAD_PARAM
    {
        ILPC* pServer;
        HANDLE hPort;
    };
    //////////////////////////////////////////////////////////////////////////
    class CLPCSender;
    class CLPCServerImpl : public ILPC, public ILPCEvent
    {
    public:

        CLPCServerImpl(ILPCEvent* pEvent);

        virtual ~CLPCServerImpl();

        // ILPC
        virtual BOOL Create(LPCTSTR lpPortName);

        virtual void Close();

        virtual ISenders* GetSenders();

        // ILPCEvent
        virtual void OnCreate(ILPC* pLPC);

        virtual void OnClose(ILPC* pLPC);

        virtual BOOL OnConnect(ILPC* pLPC, ISender* pSender);

        virtual void OnDisConnect(ILPC* pLPC, ISender* pSender);

        virtual void OnRecv(ILPC* pLPC, ISender* pSender, IMessage* pMessage);

        virtual void OnRecvAndSend(ILPC* pLPC, ISender* pSender, IMessage* pReceiveMsg, IMessage* pReplyMsg);

        // UserDefine
        void AddSender(HANDLE hPort, ISender* pSender);

        void RemoveSender(HANDLE hPort);

        ISender* FindSenderByHandle(HANDLE hPort);

        HANDLE CreateListenThread(HANDLE hPort);

    protected:

        void ClearSenders();

        static DWORD __stdcall _ListenThreadProc(LPVOID lpParam);

        DWORD _ListenThread();

        BOOL HandleConnect(PPORT_MESSAGE message);

        BOOL HandleDisConnect(HANDLE hPort);

        BOOL HandleDataGram(HANDLE hPort, IMessage* message);

        BOOL HandleRequest(HANDLE hPort, IMessage* receiveMsg, IMessage* replyMsg);

    private:

        SenderMap m_sendersMap; // 连接端映射

        ILPCEvent* m_pEvent;    // 事件接受者

        HANDLE m_hListenPort;   // LPC监听端口

        HANDLE m_hListenThread; // 监听线程句柄

        CRITICAL_SECTION m_mapCS;
    };

    class CLPCSender: public ISender
    {
    public:
        CLPCSender(HANDLE hPort, DWORD dwPID,CLPCServerImpl* pServer);

        virtual ~CLPCSender();

        void DisConnect();

        virtual DWORD GetSID();

    private:

        HANDLE m_hPort;

        CLPCServerImpl* m_pServer;

		DWORD m_dwPID;
    };

    //////////////////////////////////////////////////////////////////////////
    class CLPCSenders : public ISenders
    {
    public:
        CLPCSenders(SenderMap senderMap);

        virtual ~CLPCSenders();

        virtual void Begin();

        virtual BOOL End();

        virtual void Next();

        virtual ISender* GetCurrent();

        virtual DWORD GetSize();

    private:

        SenderMap::const_iterator m_cit;

        SenderMap m_senderMap;
    };

    class CLPCMessage: public IMessage
    {
    public:
        CLPCMessage();

        virtual ~CLPCMessage();

        virtual MESSAGE_TYPE GetMessageType();

        virtual LPVOID GetBuffer(DWORD& dwBufferSize);

        virtual void SetMessageType(MESSAGE_TYPE msgType);

        virtual void SetBuffer(LPVOID lpBuf, DWORD dwBufSize);

        PPORT_MESSAGE GetHeader();

    private:
        PORT_MESSAGE m_portHeader;

        BYTE m_lpFixBuf[256];

        DWORD m_dwBufSize;

        MESSAGE_TYPE m_msgType;
    };

}