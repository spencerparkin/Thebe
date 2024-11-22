#pragma once

#include "Reference.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API VertexBuffer : public ReferenceCounted
	{
	public:
		VertexBuffer();
		virtual ~VertexBuffer();

		// TODO: Own resource pointer.
	};
}