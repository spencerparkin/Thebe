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

		struct IndexBufferSetupData
		{
			BufferSetupData bufferSetupData;
			DXGI_FORMAT format;
			D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
		};

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

	protected:
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
	};
}