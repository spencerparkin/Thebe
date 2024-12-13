#pragma once

#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include <Windows.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * 
	 */
	class THEBE_API SwapChain : public RenderTarget
	{
	public:
		SwapChain();
		virtual ~SwapChain();

		void SetWindowHandle(HWND windowHandle);

		virtual bool Setup() override;
		virtual void Shutdown() override;

		bool Resize(int width, int height);

	protected:
		virtual bool PreRender(RenderObject::RenderContext& context) override;
		virtual bool PostRender() override;
		virtual void PreSignal() override;

		bool GetWindowDimensions(int& width, int& height);
		bool ResizeDepthBuffers(int width, int height, ID3D12Device* device);
		bool RecreateViews(ID3D12Device* device);

		HWND windowHandle;
		ComPtr<IDXGISwapChain3> swapChain;

		struct SwapFrame : public Frame
		{
			ComPtr<ID3D12Resource> renderTarget;
			ComPtr<ID3D12Resource> depthBuffer;
		};

		virtual Frame* NewFrame() override;

		CD3DX12_VIEWPORT viewport;
		CD3DX12_RECT scissorRect;
		DescriptorHeap::DescriptorSet rtvDescriptorSet;
		DescriptorHeap::DescriptorSet dsvDescriptorSet;
	};
}