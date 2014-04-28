#pragma once
#include <windows.h>
#include <tchar.h>
#include "IProcess.h"
#include "IIni.h"
#include "IFileMap.h"
#include "IMiniDump.h"
#include "IThread.h"
#include "ILPC.h"

namespace CODELIB
{
    enum INTERFACE_NAME
    {
        CODELIB_PROCESS,
        CODELIB_INIFILE,
        CODELIB_FILEMAP,
        CODELIB_MINIDUMP,
        CODELIB_THREAD,
		CODELIB_LPCSERVER,
		CODELIB_LPCCLIENT,
		CODELIB_NAMEDPIPESERVER,
		CODELIB_NAMEDPIPECLIENT
    };

    LPVOID CreateInstance(INTERFACE_NAME interfaceName,LPVOID lpCreateParam=NULL);
}
