#pragma once
#include "Common.h"

namespace CODELIB
{
	class IMiniDump
	{
	public:
		virtual ~IMiniDump()=0{};
		virtual void Active(BOOL bEnable=TRUE)=0;
	};
}