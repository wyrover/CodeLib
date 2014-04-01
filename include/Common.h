#pragma once
#include <windows.h>
#include <tchar.h>
#include "IProcess.h"

namespace CODELIB
{
    enum INTERFACE_NAME
    {
        CODELIB_PROCESS,
		CODELIB_INIFILE
    };

    LPVOID CreateInstance(INTERFACE_NAME interfaceName);
}
