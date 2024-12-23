#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/Math/Vector3.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class IndexBuffer;
	class VertexBuffer;
	class TextureBuffer;
	class UploadHeap;

	/**
	 * This is the base class for constants buffers, index buffers, vertex buffers and texture buffers.
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
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual bool CreateResourceView(CD3DX12_CPU_DESCRIPTOR_HANDLE& handle, ID3D12Device* device);

		std::vector<UINT8>& GetOriginalBuffer();
		const std::vector<UINT8>& GetOriginalBuffer() const;

		bool UpdateIfNecessary(ID3D12GraphicsCommandList* commandList);

		UINT8* GetBufferPtr();
		const UINT8* GetBufferPtr() const;
		UINT64 GetBufferSize() const;
		void SetBufferType(Type type);
		Type GetBufferType() const;
		D3D12_RESOURCE_DESC& GetResourceDesc();
		const D3D12_RESOURCE_DESC& GetResourceDesc() const;
		void SetCompressed(bool compressed);

		static bool GenerateIndexAndVertexBuffersForConvexHull(
			const std::vector<Vector3>& pointArray,
			GraphicsEngine* graphicsEngine,
			Reference<IndexBuffer>& indexBuffer,
			Reference<VertexBuffer>& vertexBuffer);

		static bool GenerateCheckerboardTextureBuffer(
			UINT width,
			UINT height,
			UINT checkerSize,
			GraphicsEngine* graphicsEngine,
			Reference<TextureBuffer>& textureBuffer);

		UINT64 GetBytesPerPixel();

	protected:
		virtual bool ValidateBufferDescription();
		virtual UINT64 GetUploadHeapAllocationSize(ID3D12Device* device);
		virtual bool CopyDataToUploadHeap(UINT8* uploadBuffer, ID3D12Device* device);
		virtual void CopyDataFromUploadHeapToDefaultHeap(UploadHeap* uploadHeap, ID3D12GraphicsCommandList* commandList, ID3D12Device* device);

		std::vector<UINT8> originalBuffer;
		Type type;
		D3D12_RESOURCE_DESC gpuBufferDesc;
		UINT64 offsetAlignmentRequirement;
		UINT64 sizeAlignmentRequirement;
		D3D12_RESOURCE_STATES resourceStateWhenRendering;
		ComPtr<ID3D12Resource> gpuBuffer;
		UINT64 uploadBufferOffset;
		UINT64 lastUpdateFrameCount;
		bool compressed;
	};
}