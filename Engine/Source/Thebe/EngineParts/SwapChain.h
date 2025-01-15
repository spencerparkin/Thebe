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
		virtual bool Render() override;
		virtual void ConfigurePiplineStateDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc) override;

		bool Resize(int width, int height);

		void SetMsaaEnabled(bool msaaEnabled);
		bool GetMsaaEnabled() const;

		const CD3DX12_VIEWPORT& GetViewport() const;

	protected:
		virtual bool PreRender(ID3D12GraphicsCommandList* commandList, RenderObject::RenderContext& context) override;
		virtual bool PostRender(ID3D12GraphicsCommandList* commandList) override;
		virtual void PreSignal() override;

		bool GetWindowDimensions(int& width, int& height);
		bool ResizeBuffers(int width, int height, ID3D12Device* device);
		bool RecreateViews(ID3D12Device* device);

		HWND windowHandle;
		ComPtr<IDXGISwapChain3> swapChain;

		class SwapFrame : public Frame
		{
		public:
			SwapFrame();
			virtual ~SwapFrame();

			ComPtr<ID3D12Resource> msaaRenderTarget;
			ComPtr<ID3D12Resource> msaaDepthBuffer;
			ComPtr<ID3D12Resource> renderTarget;
			ComPtr<ID3D12Resource> depthBuffer;
		};

		virtual Frame* NewFrame() override;

		bool CanAndShouldDoMSAA(SwapFrame* swapFrame);

		CD3DX12_VIEWPORT viewport;
		CD3DX12_RECT scissorRect;
		DescriptorHeap::DescriptorSet rtvDescriptorSet;
		DescriptorHeap::DescriptorSet dsvDescriptorSet;
		DescriptorHeap::DescriptorSet rtvMsaaDescriptorSet;
		DescriptorHeap::DescriptorSet dsvMsaaDescriptorSet;
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
		bool msaaEnabled;
	};
}