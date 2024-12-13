#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/EngineParts/Fence.h"
#include <d3d12.h>

#define THEBE_NUM_SWAP_FRAMES		2

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 *
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

	protected:
		struct Frame
		{
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			Reference<Fence> fence;
		};

		virtual Frame* NewFrame() = 0;
		virtual UINT GetCurrentFrame() = 0;

		std::vector<Frame*> frameArray;
	};
}