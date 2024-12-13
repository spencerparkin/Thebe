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
		virtual bool Render();

	protected:
		virtual bool PreRender(RenderObject::RenderContext& context);
		virtual bool PostRender();
		virtual void PreSignal();

		struct Frame
		{
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			Reference<Fence> fence;
		};

		virtual Frame* NewFrame() = 0;

		std::vector<Frame*> frameArray;
		ComPtr<ID3D12GraphicsCommandList> commandList;
	};
}