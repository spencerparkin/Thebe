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

			/**
			 * The buffer is uploaded into fast GPU memory and cannot be changed.
			 */
			STATIC,

			/**
			 * Space is reserved in fast and slow GPU memory for the buffer, the
			 * latter being mapped in CPU memory.  When an update is desired,
			 * commands are issued that transfer the memory from the slow region
			 * to the fast region of GPU memory.  Memory barriers provide the
			 * needed synchronization between the CPU and GPU.
			 */
			DYNAMIC_BARRIER_METHOD,

			/**
			 * Enough space is reserved in slow GPU memory for N copies of the
			 * buffer, where N is the number of swap-frames being used.  Space
			 * is also reserved in CPU memory for the buffer, but only as much
			 * as is required for a single copy.  When an update is desired,
			 * the CPU buffer is copied into one of the N regions that the GPU
			 * is guarenteed to not be reading from.
			 */
			DYNAMIC_N_BUFFER_METHOD
		};

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const override;

		std::vector<UINT8>& GetOriginalBuffer();
		const std::vector<UINT8>& GetOriginalBuffer() const;

		bool UpdateIfNecessary(ID3D12GraphicsCommandList* commandList);

		UINT8* GetBufferPtr();
		UINT32 GetBufferSize() const;
		void SetBufferType(Type type);
		Type GetBufferType() const;

		static bool GenerateIndexAndVertexBuffersForConvexHull(
			const std::vector<Vector3>& pointArray,
			GraphicsEngine* graphicsEngine,
			Reference<IndexBuffer>& indexBuffer,
			Reference<VertexBuffer>& vertexBuffer);

	protected:
		std::vector<UINT8> originalBuffer;
		D3D12_RESOURCE_STATES resourceStateWhenRendering;
		ComPtr<ID3D12Resource> slowMemBuffer;
		ComPtr<ID3D12Resource> fastMemBuffer;
		UINT8* gpuBufferPtr;
		std::unique_ptr<UINT8[]> cpuBuffer;
		Type type;
		UINT64 lastUpdateFrameCount;
	};
}