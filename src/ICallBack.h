#pragma once
#include <windows.h>

typedef enum REQUEST_TYPE
{
    REQUEST_KEYBOARDHOOK
};

class IRequest
{
public:
    virtual ~IRequest() = 0 {};
    virtual REQUEST_TYPE GetType() = 0;
};
class ICallBack
{
public:
    virtual ~ICallBack() = 0 {};
    virtual BOOL HandleRequest(IRequest* pRequest) = 0;
};