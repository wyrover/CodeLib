#pragma once
#include "Common.h"

namespace CODELIB
{
	class CLPCClientImpl : public ILPC
	{
	public:
		CLPCClientImpl();

		virtual ~CLPCClientImpl();

		virtual BOOL Create( LPCTSTR lpPortName);

		virtual void Close();

		virtual ISenders* GetSenders();

	};
}