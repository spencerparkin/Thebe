#pragma once

#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/Fence.h"
#include <Windows.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

#define THEBE_NUM_SWAP_FRAMES		2

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class RenderPass;

	/**
	 * 
	 */
	class THEBE_API SwapChain : public RenderTarget
	{
	public:
		SwapChain();
		virtual ~SwapChain();

		struct SetupData
		{
			HWND windowHandle;
			RenderPass* renderPass;
		};

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

		bool Resize(int width, int height);

		virtual ID3D12CommandAllocator* AcquireCommandAllocator(ID3D12CommandQueue* commandQueue) override;
		virtual void ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue);

		virtual bool PreRender(ID3D12GraphicsCommandList* commandList) override;
		virtual bool PostRender(ID3D12GraphicsCommandList* commandList) override;

	private:
		bool GetWindowDimensions(int& width, int& height);
		bool RecreateRenderTargetViews();

		HWND windowHandle;
		ComPtr<IDXGISwapChain3> swapChain;

		struct Frame
		{
			ComPtr<ID3D12Resource> renderTarget;
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			Reference<Fence> fence;
		};

		Frame frameArray[THEBE_NUM_SWAP_FRAMES];
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		CD3DX12_VIEWPORT viewport;
		CD3DX12_RECT scissorRect;
	};
}