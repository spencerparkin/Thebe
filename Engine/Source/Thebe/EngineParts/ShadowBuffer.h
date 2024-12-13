#pragma once

#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/Fence.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

#define THEBE_SHADOW_BUFFER_WIDTH			4 * 1024
#define THEBE_SHADOW_BUFFER_HEIGHT			4 * 1024

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * 
	 */
	class THEBE_API ShadowBuffer : public RenderTarget
	{
	public:
		ShadowBuffer();
		virtual ~ShadowBuffer();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool PreRender(RenderObject::RenderContext& context) override;
		virtual bool PostRender() override;

	protected:
		struct ShadowFrame : public Frame
		{
			ComPtr<ID3D12Resource> depthBuffer;
		};

		virtual Frame* NewFrame() override;

		DescriptorHeap::DescriptorSet dsvDescriptorSet;
		CD3DX12_VIEWPORT viewport;
		CD3DX12_RECT scissorRect;
	};
}