#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/Math/Vector3.h"
#include <d3d12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class IndexBuffer;
	class VertexBuffer;

	/**
	 * This is the base class for constants buffers, index buffers and vertex buffers.
	 * I'm not yet sure if it will serve as a base for textures.
	 */
	class THEBE_API Buffer : public EnginePart
	{
	public:
		Buffer();
		virtual ~Buffer();

		enum Type
		{
			NONE,
			STATIC,
			DYNAMIC
		};

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const override;

		std::vector<UINT8>& GetOriginalBuffer();
		const std::vector<UINT8>& GetOriginalBuffer() const;

		bool UpdateIfNecessary(ID3D12GraphicsCommandList* commandList);

		UINT8* GetBufferPtr();
		UINT64 GetBufferSize() const;
		void SetBufferType(Type type);
		Type GetBufferType() const;

		static bool GenerateIndexAndVertexBuffersForConvexHull(
			const std::vector<Vector3>& pointArray,
			GraphicsEngine* graphicsEngine,
			Reference<IndexBuffer>& indexBuffer,
			Reference<VertexBuffer>& vertexBuffer);

	protected:
		std::vector<UINT8> originalBuffer;
		Type type;
		UINT64 offsetAlignmentRequirement;
		UINT64 sizeAlignmentRequirement;
		D3D12_RESOURCE_STATES resourceStateWhenRendering;
		ComPtr<ID3D12Resource> gpuBuffer;
		UINT64 uploadBufferOffset;
		UINT64 lastUpdateFrameCount;
	};
}