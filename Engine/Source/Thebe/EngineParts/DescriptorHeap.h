#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/Utilities/BlockManager.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * 
	 */
	class THEBE_API DescriptorHeap : public EnginePart
	{
	public:
		DescriptorHeap();
		virtual ~DescriptorHeap();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		D3D12_DESCRIPTOR_HEAP_DESC& GetDescriptorHeapDesc();
		ID3D12DescriptorHeap* GetDescriptorHeap();

		// These descriptors are guarenteed to be contiguous in memory.
		struct DescriptorSet
		{
			friend class DescriptorHeap;

		public:
			DescriptorSet();

			bool GetCpuHandle(UINT i, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle) const;
			bool GetGpuHandle(UINT i, CD3DX12_GPU_DESCRIPTOR_HANDLE& handle) const;

		private:
			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
			UINT64 offset;
			UINT64 size;
			UINT descriptorSize;
			RefHandle heapHandle;
		};

		bool AllocDescriptorSet(UINT numDescriptors, DescriptorSet& descriptorSet);
		bool FreeDescriptorSet(DescriptorSet& descriptorSet);

	private:
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		BlockManager blockManager;
		std::map<UINT64, BlockManager::BlockNode*> blockMap;
	};
}