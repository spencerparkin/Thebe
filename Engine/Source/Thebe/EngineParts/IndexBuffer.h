#pragma once

#include "Thebe/EngineParts/Buffer.h"

namespace Thebe
{
	/**
	 *
	 */
	class THEBE_API IndexBuffer : public Buffer
	{
	public:
		IndexBuffer();
		virtual ~IndexBuffer();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void SetFormat(DXGI_FORMAT format);
		void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

	protected:
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
	};
}