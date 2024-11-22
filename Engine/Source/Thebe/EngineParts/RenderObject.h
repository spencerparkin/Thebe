#pragma once

#include "Thebe/EnginePart.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API RenderObject : public EnginePart
	{
	public:
		RenderObject();
		virtual ~RenderObject();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

		//virtual void Render(CommandList* commandList) = 0;
	};
}