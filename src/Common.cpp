#include "stdafx.h"
#include "Common.h"
#include "ProcessImpl.h"

namespace CODELIB
{
    LPVOID CreateInstance(INTERFACE_NAME interfaceName)
    {
        LPVOID pInterface = NULL;

        switch(interfaceName)
        {
            case CODELIB_PROCESS:
            {
                pInterface = new CODELIB::CProcessImpl;
            }

            default:
                break;
        }

        return pInterface;
    }
}

