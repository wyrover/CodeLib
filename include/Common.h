#pragma once
#include <windows.h>
#include <tchar.h>
#include "IProcess.h"
#include "IIni.h"
#include "IFileMap.h"

namespace CODELIB
{
    enum INTERFACE_NAME
    {
        CODELIB_PROCESS,
		CODELIB_INIFILE,
		CODELIB_FILEMAP
    };

    LPVOID CreateInstance(INTERFACE_NAME interfaceName);
}
