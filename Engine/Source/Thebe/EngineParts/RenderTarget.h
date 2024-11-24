#pragma once

#include "Thebe/EnginePart.h"
#include <d3d12.h>

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

		virtual bool Setup() override;
		virtual void Shutdown() override;

		virtual ID3D12CommandAllocator* AcquireCommandAllocator(ID3D12CommandQueue* commandQueue);
		virtual void ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue);

		virtual bool PreRender(ID3D12GraphicsCommandList* commandList);
		virtual bool PostRender(ID3D12GraphicsCommandList* commandList);
	};
}