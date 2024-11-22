#pragma once

#include "Thebe/EnginePart.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API VertexBuffer : public EnginePart
	{
	public:
		VertexBuffer();
		virtual ~VertexBuffer();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

		// TODO: Own resource pointer.
	};
}