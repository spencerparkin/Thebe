#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/EngineParts/Fence.h"
#include <d3d12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class GraphicsEngine;
	class RenderObject;
	class RenderTarget;
	class Camera;

	/**
	 * Render passes are used to render their configured input (typically a hierarchy of space
	 * nodes) into their configured output (typically a swap-chain node, but could also be a
	 * texture node for purposes of off-screen rendering, or shadow mapping.)
	 * 
	 * Ideally, these would render in parallel on the GPU with other render-passes whenever possible,
	 * only stalling for other render passes if necessary.  Stalls happen at the resoure barriers.
	 * For example, one queue stalls waiting for a resource to become readable while another queue
	 * writes to it and then makes it readable.
	 */
	class THEBE_API RenderPass : public EnginePart
	{
	public:
		RenderPass();
		virtual ~RenderPass();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;
		virtual bool Perform();

		void WaitForCommandQueueComplete();

		ID3D12CommandQueue* GetCommandQueue();
		RenderObject* GetInput();
		RenderTarget* GetOutput();

	protected:
		Reference<RenderObject> input;
		Reference<RenderTarget> output;
		Reference<Camera> camera;
		Reference<Fence> fence;
		ComPtr<ID3D12CommandQueue> commandQueue;
		ComPtr<ID3D12GraphicsCommandList> commandList;
	};
}