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

		//virtual void PreRender(CommandList* commandList) override;		// Stall if we can't render the frame yet.
		//virtual void PostRender(CommandList* commandList);				// Present/flip and signal the current frame.

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
	};
}