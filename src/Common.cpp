#include "stdafx.h"
#include "ProcessImpl.h"
#include "IniFileImpl.h"
#include "FileMapImpl.h"
#include "MiniDumpImpl.h"
#include "ThreadImpl.h"
#include "LPCServerImpl.h"
#include "LPCClientImpl.h"

namespace CODELIB
{
    LPVOID CreateInstance(INTERFACE_NAME interfaceName,LPVOID lpCreateParam)
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
                pInterface = new CODELIB::CFileMapImpl;
                break;
            }

            case CODELIB_MINIDUMP:
            {
                pInterface = new CODELIB::CMiniDumpImpl;
                break;
            }

            case CODELIB_THREAD:
            {
                pInterface = new CODELIB::CThreadImpl;
                break;
            }

			case CODELIB_LPCSERVER:
				{
					pInterface=new CODELIB::CLPCServerImpl((ILPCEvent*)lpCreateParam);
					break;
				}
			case CODELIB_LPCCLIENT:
				{
					pInterface=new CODELIB::CLPCClientImpl;
					break;
				}
            default:
                break;
        }

        return pInterface;
    }
}

