#pragma once
#include "IMiniDump.h"

namespace CODELIB
{
	class CMiniDumpImpl: public IMiniDump
	{
	public:
		CMiniDumpImpl();
		virtual ~CMiniDumpImpl();

		virtual void Active( BOOL bEnable=TRUE );
	private:
		static LONG WINAPI MyUnhandledExceptionFilter( EXCEPTION_POINTERS* ExceptionInfo );
	};
}