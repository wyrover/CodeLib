#pragma once
#include "Common.h"
#include "LPCServerImpl.h"

namespace CODELIB
{
    class CLPCClientImpl : public ILPC , public ILPCEvent
    {
    public:
        CLPCClientImpl(ILPCEvent* pEvent);

        virtual ~CLPCClientImpl();

        virtual BOOL Create(LPCTSTR lpPortName);

        virtual void Close();

        virtual void OnCreate(ILPC* pLPC);

        virtual void OnClose(ILPC* pLPC);

        virtual BOOL OnConnect(ILPC* pLPC, ISender* pSender);

        virtual void OnDisConnect(ILPC* pLPC, ISender* pSender);

        virtual void OnRecv(ILPC* pLPC, ISender* pSender, IMessage* pMessage);

        virtual void OnRecvAndReply(ILPC* pLPC, ISender* pSender, IMessage* pReceiveMsg, IMessage* pReplyMsg);

        BOOL PostData(MESSAGE_TYPE type, LPVOID lpData, DWORD dwDataSize);

        // UserDefine
        void AddSender(HANDLE hPort, ISender* pSender);

        void RemoveSender(HANDLE hPort);

        ISender* FindSenderByHandle(HANDLE hPort);

    private:
        HANDLE m_hPort;
        ILPCEvent* m_pEvent;
        SenderMap m_sendersMap; // ¡¨Ω”∂À”≥…‰
        CRITICAL_SECTION m_mapCS;
    };
}