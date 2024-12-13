#pragma once

#include "Thebe/EngineParts/CommandQueue.h"
#include "Thebe/EngineParts/Fence.h"
#include "Thebe/EngineParts/RenderObject.h"
#include <d3d12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class GraphicsEngine;
	class RenderObject;
	class RenderTarget;
	class Camera;
	class Light;

	/**
	 * Ideally these would render in parallel with one another as much as possible,
	 * waiting only for resource barriers when necessary.
	 */
	class THEBE_API RenderPass : public CommandQueue
	{
	public:
		RenderPass();
		virtual ~RenderPass();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render();

		RenderTarget* GetRenderTarget();
		void SetRenderTarget(RenderTarget* renderTarget);

	protected:
		virtual bool GetRenderContext(RenderObject::RenderContext& context);
		virtual RenderObject* GetRenderObject();

		Reference<RenderTarget> renderTarget;
		ComPtr<ID3D12GraphicsCommandList> commandList;
	};
}