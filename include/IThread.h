#pragma once
#include "Common.h"
namespace CODELIB
{
    class IThread
    {
    public:
        virtual ~IThread() = 0 {};
        virtual BOOL Start() = 0;
        virtual BOOL Stop() = 0;
        virtual BOOL Run() = 0;
    };
}