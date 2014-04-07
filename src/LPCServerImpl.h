#pragma once
#include "Common.h"
#include <map>
#include "ILPC.h"

namespace CODELIB
{
    typedef std::map<HANDLE, ISender*> SenderMap;
    static const int FIXEDBUFLEN = 256;

    class CLPCMessage : public IMessage
    {
    public:

        CLPCMessage();

        virtual ~CLPCMessage();

        virtual MESSAGE_TYPE GetMessageType();

        virtual LPVOID GetBuffer(DWORD& dwBufferSize);

        virtual void SetMessageType(MESSAGE_TYPE messageType);

        virtual void SetBuffer(LPVOID lpBuf, DWORD dwBufSize);

        PPORT_MESSAGE GetHeader();

        void SetHeader(PORT_MESSAGE lpcHeader);

    protected:

        BOOL IsUseSectionView();

    private:

        PORT_MESSAGE m_LpcHeader;

        BYTE m_lpFixedBuf[FIXEDBUFLEN];

        DWORD m_dwBufSize;

        MESSAGE_TYPE m_messageType;

        BOOL m_bUseSectionView;
    };

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

        // UserDefine

        CLPCSender* AddSender(HANDLE hPort);

        void AddSender(HANDLE hPort, ISender* pSender);

        void RemoveSender(HANDLE hPort);

        ISender* FindSenderByHandle(HANDLE hPort);

        HANDLE CreateListenThread(HANDLE hPort);

        HANDLE GetListenPortHandle();

    protected:

        void ClearSenders();

        static DWORD __stdcall _ListenThreadProc(LPVOID lpParam);

        DWORD _ListenThread(HANDLE hPort);

        BOOL HandleConnect(CLPCMessage* connectInfo);

        BOOL HandleDisConnect(HANDLE hPort);

        BOOL HandleRequest(HANDLE hPort, CLPCMessage* recevieData, CLPCMessage* replyData);

    private:

        SenderMap m_sendersMap; // 连接端映射

        ILPCEvent* m_pEvent;    // 事件接受者

        CRITICAL_SECTION m_mapCS;

        HANDLE m_hListenPort;

        HANDLE m_hListenThread;
    };

    class CLPCSender: public ISender
    {
    public:
        CLPCSender(HANDLE hPort);

        virtual ~CLPCSender();

        BOOL Connect(CLPCServerImpl* pServer);

        void DisConnect(CLPCServerImpl* pServer);

        virtual DWORD GetSID();

        virtual BOOL SendMessage(IMessage* pMessage);

        virtual BOOL AllocMessage(IMessage* pMessage);

        virtual void FreeMessage(IMessage* pMessage);

    private:

        HANDLE m_hListenThread;

        HANDLE m_hPort;

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





}