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

		void SetWindowHandle(HWND windowHandle);
		void SetCommandQueue(ID3D12CommandQueue* commandQueue);

		virtual bool Setup() override;
		virtual void Shutdown() override;

		bool Resize(int width, int height);

		virtual ID3D12CommandAllocator* AcquireCommandAllocator(ID3D12CommandQueue* commandQueue) override;
		virtual void ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue);

		virtual bool PreRender(ID3D12GraphicsCommandList* commandList) override;
		virtual bool PostRender(ID3D12GraphicsCommandList* commandList) override;

		int GetCurrentBackBufferIndex();

	private:
		bool GetWindowDimensions(int& width, int& height);
		bool ResizeDepthBuffers(int width, int height, ID3D12Device* device);
		bool RecreateViews(ID3D12Device* device);

		HWND windowHandle;
		ID3D12CommandQueue* commandQueueForSwapChainCreate;
		ComPtr<IDXGISwapChain3> swapChain;

		struct Frame
		{
			ComPtr<ID3D12Resource> renderTarget;
			ComPtr<ID3D12Resource> depthBuffer;
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			Reference<Fence> fence;
		};

		Frame frameArray[THEBE_NUM_SWAP_FRAMES];
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		ComPtr<ID3D12DescriptorHeap> dsvHeap;
		CD3DX12_VIEWPORT viewport;
		CD3DX12_RECT scissorRect;
	};
}