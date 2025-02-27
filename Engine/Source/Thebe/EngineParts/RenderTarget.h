#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/EngineParts/CommandAllocator.h"
#include "Thebe/EngineParts/RenderObject.h"
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
		virtual bool Render();
		virtual void ConfigurePiplineStateDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc);

		uint32_t GetNumFrames();

	protected:
		virtual bool PreRender(ID3D12GraphicsCommandList* commandList, RenderObject::RenderContext& context);
		virtual bool PostRender(ID3D12GraphicsCommandList* commandList);
		virtual void PreSignal();

		class Frame : public CommandAllocator
		{
		public:
			Frame();
			virtual ~Frame();

			virtual void PreSignal() override;

			void SetRenderTargetOwner(RenderTarget* renderTargetOwner);
			void SetFrameNumber(UINT frameNumber);

		protected:
			RenderTarget* renderTargetOwner;
			UINT frameNumber;
		};

		virtual Frame* NewFrame() = 0;

		std::vector<Reference<Frame>> frameArray;
	};
}