#pragma once

#include "Thebe/EngineParts/Buffer.h"
#include "Thebe/Utilities/ScratchHeap.h"

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

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;

		void SetStride(UINT32 stride);

		std::vector<D3D12_INPUT_ELEMENT_DESC>& GetElementDescArray();
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetElementDescArray() const;
		const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const;

	protected:
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		std::vector<D3D12_INPUT_ELEMENT_DESC> elementDescArray;
		ScratchHeap semanticNameHeap;
	};
}