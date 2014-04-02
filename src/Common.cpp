#include "stdafx.h"
#include "Common.h"
#include "ProcessImpl.h"
#include "IniFileImpl.h"
#include "FileMapImpl.h"

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
                break;
            }

            case CODELIB_INIFILE:
            {
                pInterface = new CODELIB::CIniFileImpl;
                break;
            }
			case CODELIB_FILEMAP:
				{
					pInterface=new CODELIB::CFileMapImpl;
					break;
				}

            default:
                break;
        }

        return pInterface;
    }
}

