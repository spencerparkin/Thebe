#include "Thebe/Common.h"
#include "Thebe/Log.h"
#include <Windows.h>
#include <debugapi.h>

namespace Thebe
{
	void Assert(bool condition, const char* conditionStr, const char* sourceFile, int lineNumber, bool fatal)
	{
		if (!condition)
		{
			THEBE_LOG("ASSERTION FAILURE: %s", conditionStr);
			THEBE_LOG("File: %s", sourceFile);
			THEBE_LOG("Line: %d", lineNumber);

			if (::IsDebuggerPresent())
			{
				::DebugBreak();
			}
			else if (fatal)
			{
				::ExitProcess(1);
			}
		}
	}
}