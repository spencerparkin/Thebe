#pragma once

#include "Node.h"

namespace Thebe
{
	/**
	 * This is anything that can be a target for rendering, such as
	 * a texture or swap-chain.
	 */
	class THEBE_API RenderTarget : public Node
	{
	public:
		RenderTarget();
		virtual ~RenderTarget();

		//virtual void PrepareForRendering(CommandList* commandList);
	};
}