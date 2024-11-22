#pragma once

#include "Reference.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API RenderObject : public ReferenceCounted
	{
	public:
		RenderObject();
		virtual ~RenderObject();

		//virtual void Render(CommandList* commandList);
	};
}