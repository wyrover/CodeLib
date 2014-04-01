#pragma once
#include <windows.h>
#include "IProcess.h"

namespace CODELIB
{
    enum INTERFACE_NAME
    {
        CODELIB_PROCESS
    };

    LPVOID CreateInstance(INTERFACE_NAME interfaceName);
}
