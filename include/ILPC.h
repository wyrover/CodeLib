#pragma once
#include "ntdll.h"

namespace CODELIB
{
    enum MESSAGE_TYPE
    {
        LPC_MESSAGE_REQUEST,
        LPC_MESSAGE_REPLY
    };

    class IMessage
    {
    public:
        virtual ~IMessage() = 0 {};
        virtual PPORT_MESSAGE GetHeader() = 0;
        virtual MESSAGE_TYPE GetMessageType() = 0;
        virtual LPVOID GetBuffer(DWORD& dwBufferSize) = 0;
        virtual void SetMessageType(MESSAGE_TYPE msgType) = 0;
        virtual void SetBuffer(LPVOID lpBuf, DWORD dwBufSize) = 0;
    };

    class ISender
    {
    public:
        virtual ~ISender() = 0 {};
        virtual DWORD GetSID() = 0;
        virtual BOOL SendMessage(IMessage* pMessage) = 0;
        virtual BOOL PostMessage(IMessage* pMessage) = 0;
    };

    class ILPC
    {
    public:
        virtual ~ILPC() = 0 {};
        virtual BOOL Create(LPCTSTR lpPortName) = 0;
        virtual void Close() = 0;
    };

    class ILPCEvent
    {
    public:
        virtual ~ILPCEvent() = 0 {};
        virtual void OnCreate(ILPC* pLPC) = 0;
        virtual void OnClose(ILPC* pLPC) = 0;
        virtual BOOL OnConnect(ILPC* pLPC, ISender* pSender) = 0;
        virtual void OnDisConnect(ILPC* pLPC, ISender* pSender) = 0;
        virtual void OnRecv(ILPC* pLPC, ISender* pSender, IMessage* pMessage) = 0;
        virtual void OnRecvAndReply(ILPC* pLPC, ISender* pSender, IMessage* pReceiveMsg, IMessage* pReplyMsg) = 0;
    };
}