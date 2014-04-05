#pragma once
#include "ntdll.h"

namespace CODELIB
{
    enum MESSAGE_TYPE
    {

    };

    class IMessage
    {
    public:
        virtual ~IMessage() = 0 {};
        virtual MESSAGE_TYPE GetMessageType() = 0;
        virtual LPVOID GetBuffer(DWORD& dwBufferSize) = 0;
		virtual void SetMessageType()=0;
		virtual void SetBuffer(LPVOID lpBuf,DWORD dwBufSize)=0;
    };

	typedef struct _TRANSFERRED_DATA
	{
		_TRANSFERRED_DATA()
		{
			InitializeMessageHeader(&Header, sizeof(_TRANSFERRED_DATA), 0);
		}
		PORT_MESSAGE            Header;
		IMessage* pMessage;
	} TRANSFERRED_DATA, *PTRANSFERRED_DATA;

    class ISender
    {
    public:
        virtual ~ISender() = 0 {};
        virtual DWORD GetSID() = 0;
		virtual IMessage* AllocMessage()=0;
		virtual void FreeMessage(IMessage* pMessage)=0;
        virtual BOOL SendMessage(IMessage* pMessage) = 0;
    };

    class ISenders
    {
    public:
        virtual ~ISenders() = 0 {};
        virtual void Begin() = 0;
        virtual BOOL End() = 0;
        virtual void Next() = 0;
        virtual ISender* GetCurrent() = 0;
        virtual DWORD GetSize() = 0;
    };

    class ILPC
    {
    public:
        virtual ~ILPC() = 0 {};
        virtual BOOL Create(LPCTSTR lpPortName) = 0;
        virtual void Close() = 0;
        virtual ISenders* GetSenders() = 0;
    };

    class ILPCEvent
    {
    public:
        virtual ~ILPCEvent() = 0 {};
        virtual void OnCreate(ILPC* pLPC) = 0;
        virtual void OnClose(ILPC* pLPC) = 0;
        virtual BOOL OnConnect(ILPC* pLPC, ISender* pSender) = 0;
        virtual void OnDisConnect(ILPC* pLPC, ISender* pSender) = 0;
        virtual void OnRecv(ILPC* pLPC, ISender* pSender,IMessage* pMessage) = 0;
    };
}