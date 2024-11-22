#pragma once

#include "Thebe/EnginePart.h"

namespace Thebe
{
	/**
	 * This is anything that can be a target for rendering, such as
	 * a texture or swap-chain.
	 */
	class THEBE_API RenderTarget : public EnginePart
	{
	public:
		RenderTarget();
		virtual ~RenderTarget();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

		//virtual void PreRender(CommandList* commandList);		// Issue resource barrier from previous to writable.
		//virtual void PostRender(CommandList* commandList);	// Issue resource barrior from previous to readable?
		//virtual ID3D12CommandAllocator* GetCommandAllocator();
	};
}