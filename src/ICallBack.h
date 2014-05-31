#pragma once
#include <windows.h>

typedef enum REQUEST_TYPE
{
    REQUEST_KEYBOARDHOOK,
    REQUEST_MEMSCAN
};

class IRequest
{
public:
    virtual ~IRequest() = 0 {};
    virtual REQUEST_TYPE GetType() = 0;
};

class CRequestBase : public IRequest
{
public:
    CRequestBase(REQUEST_TYPE requestType): m_requestType(requestType) {}

    virtual ~CRequestBase() {}

    virtual REQUEST_TYPE GetType()
    {
        return m_requestType;
    }

private:
    REQUEST_TYPE m_requestType;
};
class IRequestHandler
{
public:
    virtual ~IRequestHandler() = 0 {};
    virtual BOOL HandleRequest(IRequest* pRequest) = 0;
};