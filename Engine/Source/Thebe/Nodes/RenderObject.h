#pragma once

#include "Node.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API RenderObject : public Node
	{
	public:
		RenderObject();
		virtual ~RenderObject();

		//virtual void Render(CommandList* commandList);
	};
}