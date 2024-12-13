#pragma once

#include "Thebe/EngineParts/CommandQueue.h"
#include "Thebe/EngineParts/Fence.h"
#include "Thebe/EngineParts/RenderObject.h"
#include <d3d12.h>

#define THEBE_NUM_SWAP_FRAMES		2

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 *
	 */
	class THEBE_API RenderTarget : public CommandQueue
	{
	public:
		RenderTarget();
		virtual ~RenderTarget();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		virtual ID3D12CommandAllocator* AcquireCommandAllocator(ID3D12CommandQueue* commandQueue);
		virtual void ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue);
		virtual bool Render();
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
		virtual bool GetRenderContext(RenderObject::RenderContext& context);
		virtual RenderObject* GetRenderObject();

		std::vector<Frame*> frameArray;
		ComPtr<ID3D12GraphicsCommandList> commandList;
	};
}