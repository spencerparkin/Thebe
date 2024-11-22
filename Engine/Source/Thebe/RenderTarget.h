#pragma once

#include "Reference.h"

namespace Thebe
{
	/**
	 * This is anything that can be a target for rendering, such as
	 * a texture or swap-chain.
	 */
	class THEBE_API RenderTarget : public ReferenceCounted
	{
	public:
		RenderTarget();
		virtual ~RenderTarget();

		//virtual void PreRender(CommandList* commandList);		// Issue resource barrier from previous to writable.
		//virtual void PostRender(CommandList* commandList);	// Issue resource barrior from previous to readable?
	};
}