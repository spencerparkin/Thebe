#pragma once

#include "Thebe/EnginePart.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * This is similar to the DescriptorHeap class, but here we only allocate
	 * and deallocate one descriptor at a time.
	 */
	class THEBE_API DescriptorPool : public EnginePart
	{
	public:
		DescriptorPool();
		virtual ~DescriptorPool();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		D3D12_DESCRIPTOR_HEAP_DESC& GetDescriptorHeapDesc();
		ID3D12DescriptorHeap* GetDescriptorHeap();

		// Unlike the counter-part to this in the DescriptorHeap class, you don't
		// need to pass in the original when performing deallocation.  You can just
		// create a new instance of this class, fill it out, and then ask for deallocation.
		struct Descriptor
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		};

		bool AllocDescriptor(Descriptor& descriptor);
		bool FreeDescriptor(const Descriptor& descriptor);

	private:
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		std::vector<uint64_t> descriptorOffsetStack;
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBaseHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBaseHandle;
		uint64_t incrementSize;
	};
}