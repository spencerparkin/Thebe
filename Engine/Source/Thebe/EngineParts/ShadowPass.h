#pragma once

#include "Thebe/EngineParts/RenderPass.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API ShadowPass : public RenderPass
	{
	public:
		ShadowPass();
		virtual ~ShadowPass();

		virtual bool Setup() override;
		virtual void Shutdown() override;

	protected:
		virtual bool GetRenderContext(RenderObject::RenderContext& context) override;
	};
}