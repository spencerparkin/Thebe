#pragma once

#if defined THEBE_EXPORT
#	define THEBE_API		__declspec(dllexport)
#elif defined THEBE_IMPORT
#	define THEBE_API		__declspec(dllimport)
#else
#	define THEBE_API
#endif

namespace Thebe
{
	class THEBE_API Engine
	{
	public:
		Engine();
		virtual ~Engine();
	};
}