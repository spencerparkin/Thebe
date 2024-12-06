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

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const override;

		void SetStride(UINT32 stride);

		std::vector<D3D12_INPUT_ELEMENT_DESC>& GetElementDescArray();
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetElementDescArray() const;
		const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const;

		void ClearElementDescriptionArray();

	protected:
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		std::vector<D3D12_INPUT_ELEMENT_DESC> elementDescArray;
	};
}