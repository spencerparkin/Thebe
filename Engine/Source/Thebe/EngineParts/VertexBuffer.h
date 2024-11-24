#pragma once

#include "Thebe/EngineParts/Buffer.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API VertexBuffer : public Buffer
	{
	public:
		VertexBuffer();
		virtual ~VertexBuffer();

		struct VertexBufferSetupData
		{
			BufferSetupData bufferSetupData;
			UINT32 strideInBytes;
			std::vector<D3D12_INPUT_ELEMENT_DESC> elementDescArray;
		};

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

		std::vector<D3D12_INPUT_ELEMENT_DESC>& GetElementDescArray();
		const std::vector< D3D12_INPUT_ELEMENT_DESC>& GetElementDescArray() const;

	protected:
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		std::vector<D3D12_INPUT_ELEMENT_DESC> elementDescArray;
	};
}