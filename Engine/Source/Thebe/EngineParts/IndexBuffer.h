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
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const override;

		void SetFormat(DXGI_FORMAT format);
		void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);
		D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const;
		const D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const;

		UINT GetIndicesPerInstance() const;
		UINT GetInstanceCount() const;
		UINT GetStride() const;
		UINT GetIndexCount() const;

	protected:
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
	};
}