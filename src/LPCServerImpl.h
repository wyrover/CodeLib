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

        void AddSender(HANDLE hPort, ISender* pSender);

        void RemoveSender(HANDLE hPort);

        ISender* FindSenderByHandle(HANDLE hPort);

		void ClearSenders();

		HANDLE CreateListenThread(HANDLE hPort);

		HANDLE GetListenPort();
    protected:
        
        static DWORD __stdcall _ListenThreadProc(LPVOID lpParam);

        DWORD _ListenThread(HANDLE hPort);

        BOOL HandleConnect(PPORT_MESSAGE message);

        BOOL HandleDisConnect(HANDLE hPort);
    private:
        SenderMap m_sendersMap; // 连接端映射
        ILPCEvent* m_pEvent;    // 事件接受者
        HANDLE m_hListenPort;   // LPC监听端口
        HANDLE m_hListenThread; // 监听线程句柄
    };

    class CLPCSender: public ISender
    {
    public:
        CLPCSender(HANDLE hPort,CLPCServerImpl* pServer);
        virtual ~CLPCSender();

        BOOL Connect();

		void DisConnect(BOOL bSelfExit=FALSE);

        virtual DWORD GetSID();

        virtual IMessage* AllocMessage();

        virtual void FreeMessage(IMessage* pMessage);

        virtual BOOL SendMessage(IMessage* pMessage);

		HANDLE GetHandle();
    private:
		HANDLE m_hListenThread;
        HANDLE m_hPort;
		CLPCServerImpl* m_pServer;
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