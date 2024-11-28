#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/Utilities/BlockManager.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include <map>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * 
	 */
	class THEBE_API UploadHeap : public EnginePart
	{
	public:
		UploadHeap();
		virtual ~UploadHeap();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		bool Allocate(UINT64 size, UINT64 align, UINT64& offset);
		bool Deallocate(UINT64);

		void SetUploadBufferSize(UINT64 uploadBufferSize);
		UINT8* GetUploadBufferPtr();

	private:
		UINT64 uploadBufferSize;
		ComPtr<ID3D12Resource> uploadBuffer;
		BlockManager blockManager;
		UINT8* uploadBufferMapped;
		std::map<UINT64, BlockManager::BlockNode*> blockMap;
	};
}