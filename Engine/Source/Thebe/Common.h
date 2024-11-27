#pragma once

#if defined THEBE_EXPORT
#	define THEBE_API		__declspec(dllexport)
#elif defined THEBE_IMPORT
#	define THEBE_API		__declspec(dllimport)
#else
#	define THEBE_API
#endif

#include <assert.h>
#include <vector>
#include <list>

#define THEBE_MIN(a, b)			((a) < (b) ? (a) : (b))
#define THEBE_MAX(a, b)			((a) > (b) ? (a) : (b))
#define THEBE_CLAMP(x, a, b)	THEBE_MAX(THEBE_MIN(x, b), a)
#define THEBE_SQUARED(x)		((x) * (x))
#define THEBE_SIGN(x)			((x) < 0.0 ? -1.0 : 1.0)
#define THEBE_DEGS_TO_RADS(x)	((x) * (M_PI / 180.0))
#define THEBE_RADS_TO_DEGS(x)	((x) * (180.0 / M_PI))
#define THEBE_IS_POW_TWO(x)		((x & (x - 1)) == 0)
#define THEBE_ALIGNED(x, y)		(((x) + ((y) - 1)) & ~((y) - 1))

#define THEBE_ASSERT			assert

#define THEBE_AXIS_FLAG_X		0x00000001
#define THEBE_AXIS_FLAG_Y		0x00000002
#define THEBE_AXIS_FLAG_Z		0x00000004