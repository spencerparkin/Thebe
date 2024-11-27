#pragma once

#include "Thebe/EnginePart.h"
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

		struct Descriptor
		{
			Descriptor();

			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
			UINT i;
			RefHandle heapHandle;
		};

		bool AllocDescriptor(Descriptor& descriptor);
		bool FreeDescriptor(Descriptor& descriptor);

	private:
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		std::vector<UINT> freeDescriptorsStack;
	};
}