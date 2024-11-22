#pragma once

#include "Thebe/EngineParts/RenderPass.h"

namespace Thebe
{
	/**
	 * The main render pass composites all off-screen rendering done by other render passes.
	 */
	class THEBE_API MainRenderPass : public RenderPass
	{
	public:
		MainRenderPass();
		virtual ~MainRenderPass();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;
	};
}