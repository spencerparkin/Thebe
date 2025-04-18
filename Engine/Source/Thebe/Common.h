#pragma once

#if defined THEBE_EXPORT
#	define THEBE_API		__declspec(dllexport)
#elif defined THEBE_IMPORT
#	define THEBE_API		__declspec(dllimport)
#else
#	define THEBE_API
#endif

#include <vector>
#include <list>

#define THEBE_MIN(a, b)				((a) < (b) ? (a) : (b))
#define THEBE_MAX(a, b)				((a) > (b) ? (a) : (b))
#define THEBE_CLAMP(x, a, b)		THEBE_MAX(THEBE_MIN(x, b), a)
#define THEBE_SQUARED(x)			((x) * (x))
#define THEBE_SIGN(x)				((x) < 0.0 ? -1.0 : 1.0)
#define THEBE_DEGS_TO_RADS(x)		((x) * (M_PI / 180.0))
#define THEBE_RADS_TO_DEGS(x)		((x) * (180.0 / M_PI))
#define THEBE_IS_POW_TWO(x)			((x & (x - 1)) == 0)
#define THEBE_ALIGNED(x, y)			(((x) + ((y) - 1)) & ~((y) - 1))
#define THEBE_PI					M_PI
#define THEBE_PHI					1.618033988749
#define THEBE_SMALL_EPS				1e-9
#define THEBE_MEDIUM_EPS			1e-5
#define THEBE_FAT_EPS				1e-2
#define THEBE_OBESE_EPS				0.1

#if defined _DEBUG
#	define THEBE_ASSERT(x)			do { Thebe::Assert(x, #x, __FILE__, __LINE__, false); } while(false)
#	define THEBE_ASSERT_FATAL(x)	do { Thebe::Assert(x, #x, __FILE__, __LINE__, true); } while(false)
#else
#	define THEBE_ASSERT(x)
#	define THEBE_ASSERT_FATAL(x)
#endif

#define THEBE_AXIS_FLAG_X			0x00000001
#define THEBE_AXIS_FLAG_Y			0x00000002
#define THEBE_AXIS_FLAG_Z			0x00000004

namespace Thebe
{
	THEBE_API void Assert(bool condition, const char* conditionStr, const char* sourceFile, int lineNumber, bool fatal);
}