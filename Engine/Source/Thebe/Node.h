#pragma once

#include "Common.h"
#include "Reference.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API Node : ReferenceCounted
	{
	public:
		Node();
		virtual ~Node();
	};
}