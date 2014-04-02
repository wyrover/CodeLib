#pragma once
#include <windows.h>
#include <tchar.h>
#include "IProcess.h"
#include "IIni.h"
#include "IFileMap.h"
#include "IMiniDump.h"

namespace CODELIB
{
    enum INTERFACE_NAME
    {
        CODELIB_PROCESS,
        CODELIB_INIFILE,
        CODELIB_FILEMAP,
        CODELIB_MINIDUMP
    };

    LPVOID CreateInstance(INTERFACE_NAME interfaceName);
}
