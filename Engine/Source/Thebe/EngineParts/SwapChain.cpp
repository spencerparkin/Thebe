#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/CommandQueue.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------------------------ SwapChain ------------------------------------------------

SwapChain::SwapChain()
{
	::memset(&this->qualityLevels, 0, sizeof(this->qualityLevels));

	this->windowHandle = NULL;
	this->msaaEnabled = true;

	this->SetName("SwapChain");
}

/*virtual*/ SwapChain::~SwapChain()
{
}

void SwapChain::SetMsaaEnabled(bool msaaEnabled)
{
	this->msaaEnabled = msaaEnabled;
}

bool SwapChain::GetMsaaEnabled() const
{
	return this->msaaEnabled;
}

/*virtual*/ RenderTarget::Frame* SwapChain::NewFrame()
{
	return new SwapFrame();
}

void SwapChain::SetWindowHandle(HWND windowHandle)
{
	this->windowHandle = windowHandle;
}

/*virtual*/ bool SwapChain::Setup()
{
	if (!this->windowHandle)
	{
		THEBE_LOG("No window handle configured.");
		return false;
	}

	if (!RenderTarget::Setup())
		return false;

	if (this->frameArray.size() != THEBE_NUM_SWAP_FRAMES)
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	IDXGIFactory4* factory = graphicsEngine->GetFactory();

	int width = 0, height = 0;
	if (!this->GetWindowDimensions(width, height))
	{
		THEBE_LOG("Failed to get window dimensions.");
		return false;
	}

	this->viewport.TopLeftX = 0;
	this->viewport.TopLeftY = 0;
	this->viewport.Width = width;
	this->viewport.Height = height;
	this->viewport.MinDepth = 0.0;
	this->viewport.MaxDepth = 1.0;

	this->scissorRect.left = 0;
	this->scissorRect.right = width;
	this->scissorRect.top = 0;
	this->scissorRect.bottom = height;

	HRESULT result = 0;

	::memset(&this->qualityLevels, 0, sizeof(this->qualityLevels));
	this->qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	this->qualityLevels.SampleCount = 16;
	while (this->qualityLevels.SampleCount > 0)
	{
		result = device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &this->qualityLevels, sizeof(this->qualityLevels));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to check for MSAA feature support.");
			return false;
		}

		if (this->qualityLevels.NumQualityLevels > 0)
			break;

		this->qualityLevels.SampleCount >>= 1;
	}

	ComPtr<IDXGISwapChain1> swapChain1;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = THEBE_NUM_SWAP_FRAMES;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	result = factory->CreateSwapChainForHwnd(
		graphicsEngine->GetCommandQueue()->GetCommandQueue(),
		this->windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create swap-chain.  Error: 0x%08x", result);
		return false;
	}

	result = swapChain1.As(&this->swapChain);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to cast swap-chain to desired type.  Error: 0x%08x", result);
		return false;
	}

	if (!graphicsEngine->GetRTVDescriptorHeap()->AllocDescriptorSet(THEBE_NUM_SWAP_FRAMES, this->rtvDescriptorSet))
	{
		THEBE_LOG("Failed to allocate RTV descriptor set for swap chain.");
		return false;
	}

	if (!graphicsEngine->GetDSVDescriptorHeap()->AllocDescriptorSet(THEBE_NUM_SWAP_FRAMES, this->dsvDescriptorSet))
	{
		THEBE_LOG("Failed to allocate DSV descriptor set for swap chain.");
		return false;
	}

	if (!graphicsEngine->GetRTVDescriptorHeap()->AllocDescriptorSet(THEBE_NUM_SWAP_FRAMES, this->rtvMsaaDescriptorSet))
	{
		THEBE_LOG("Failed to allocate RTV MSAA descriptor set for swap chain.");
		return false;
	}

	if (!graphicsEngine->GetDSVDescriptorHeap()->AllocDescriptorSet(THEBE_NUM_SWAP_FRAMES, this->dsvMsaaDescriptorSet))
	{
		THEBE_LOG("Failed to allocate DSV MSAA descriptor set for swap chain.");
		return false;
	}

	if (!this->ResizeBuffers(width, height, device))
	{
		THEBE_LOG("Failed to create depth bufferes.");
		return false;
	}

	if (!this->RecreateViews(device))
	{
		THEBE_LOG("Failed to create render target views into each swap-frame.");
		return false;
	}

	return true;
}

/*virtual*/ void SwapChain::Shutdown()
{
	this->swapChain = nullptr;

	Reference<GraphicsEngine> graphicsEngine;
	if (this->GetGraphicsEngine(graphicsEngine))
	{
		if (this->rtvDescriptorSet.IsAllocated())
			graphicsEngine->GetRTVDescriptorHeap()->FreeDescriptorSet(this->rtvDescriptorSet);

		if (this->dsvDescriptorSet.IsAllocated())
			graphicsEngine->GetDSVDescriptorHeap()->FreeDescriptorSet(this->dsvDescriptorSet);

		if (this->rtvMsaaDescriptorSet.IsAllocated())
			graphicsEngine->GetRTVDescriptorHeap()->FreeDescriptorSet(this->rtvMsaaDescriptorSet);

		if (this->dsvMsaaDescriptorSet.IsAllocated())
			graphicsEngine->GetDSVDescriptorHeap()->FreeDescriptorSet(this->dsvMsaaDescriptorSet);
	}

	for (int i = 0; i < (int)this->frameArray.size(); i++)
	{
		auto frame = (SwapFrame*)this->frameArray[i].Get();
		frame->depthBuffer = nullptr;
		frame->renderTarget = nullptr;
		frame->msaaDepthBuffer = nullptr;
		frame->msaaRenderTarget = nullptr;
	}

	RenderTarget::Shutdown();
}

/*virtual*/ bool SwapChain::Render()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (graphicsEngine->GetFrameIndex() != this->swapChain->GetCurrentBackBufferIndex())
		return true;	// No problem.  Just wait until they match up again.

	return RenderTarget::Render();
}

bool SwapChain::RecreateViews(ID3D12Device* device)
{
	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		auto frame = (SwapFrame*)this->frameArray[i].Get();

		HRESULT result = this->swapChain->GetBuffer(i, IID_PPV_ARGS(&frame->renderTarget));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to get render target for frame %d.  Error: 0x%08x", i, result);
			return false;
		}

		wchar_t renderTargetName[128];
		wsprintfW(renderTargetName, L"Swap Frame Render Target %d", i);
		frame->renderTarget->SetName(renderTargetName);

		wchar_t depthBufferName[128];
		wsprintfW(depthBufferName, L"Depth Render Target %d", i);
		frame->depthBuffer->SetName(depthBufferName);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
		this->rtvDescriptorSet.GetCpuHandle(i, rtvHandle);
		device->CreateRenderTargetView(frame->renderTarget.Get(), nullptr, rtvHandle);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
		this->dsvDescriptorSet.GetCpuHandle(i, dsvHandle);
		device->CreateDepthStencilView(frame->depthBuffer.Get(), nullptr, dsvHandle);

		if (frame->msaaRenderTarget.Get())
		{
			wchar_t msaaRenderTargetName[128];
			wsprintfW(msaaRenderTargetName, L"MSAA Swap Frame Render Target %d", i);
			frame->msaaRenderTarget->SetName(msaaRenderTargetName);

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvMsaaHandle;
			this->rtvMsaaDescriptorSet.GetCpuHandle(i, rtvMsaaHandle);
			device->CreateRenderTargetView(frame->msaaRenderTarget.Get(), nullptr, rtvMsaaHandle);
		}

		if (frame->msaaDepthBuffer.Get())
		{
			wchar_t msaaDepthBufferName[128];
			wsprintfW(msaaDepthBufferName, L"MSAA Depth Render Target %d", i);
			frame->msaaDepthBuffer->SetName(msaaDepthBufferName);

			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvMsaaHandle;
			this->dsvMsaaDescriptorSet.GetCpuHandle(i, dsvMsaaHandle);
			device->CreateDepthStencilView(frame->msaaDepthBuffer.Get(), nullptr, dsvMsaaHandle);
		}
	}

	return true;
}

DXGI_FORMAT SwapChain::GetRenderTargetFormat()
{
	if (this->frameArray.size() == 0)
		return DXGI_FORMAT_UNKNOWN;

	D3D12_RESOURCE_DESC desc{};

	auto frame = (SwapFrame*)this->frameArray[0].Get();

	if (this->CanAndShouldDoMSAA(frame))
		desc = frame->renderTarget->GetDesc();
	else
		desc = frame->msaaRenderTarget->GetDesc();

	return desc.Format;
}

DXGI_FORMAT SwapChain::GetDepthStencileViewFormat()
{
	if (this->frameArray.size() == 0)
		return DXGI_FORMAT_UNKNOWN;

	D3D12_RESOURCE_DESC desc{};

	auto frame = (SwapFrame*)this->frameArray[0].Get();

	if (this->CanAndShouldDoMSAA(frame))
		desc = frame->msaaDepthBuffer->GetDesc();
	else
		desc = frame->depthBuffer->GetDesc();

	return desc.Format;
}

bool SwapChain::ResizeBuffers(int width, int height, ID3D12Device* device)
{
	HRESULT result = 0;

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	D3D12_CLEAR_VALUE renderTargetClearValue{};
	renderTargetClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	renderTargetClearValue.Color[0] = 0.0f;
	renderTargetClearValue.Color[1] = 0.0f;
	renderTargetClearValue.Color[2] = 0.0f;
	renderTargetClearValue.Color[3] = 1.0f;

	D3D12_RESOURCE_DESC msaaRenderTargetDesc{};
	msaaRenderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	msaaRenderTargetDesc.Alignment = 0;
	msaaRenderTargetDesc.Width = width;
	msaaRenderTargetDesc.Height = height;
	msaaRenderTargetDesc.DepthOrArraySize = 1;
	msaaRenderTargetDesc.MipLevels = 1;
	msaaRenderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msaaRenderTargetDesc.SampleDesc.Count = this->qualityLevels.SampleCount;
	msaaRenderTargetDesc.SampleDesc.Quality = this->qualityLevels.NumQualityLevels - 1;
	msaaRenderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	msaaRenderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_RESOURCE_DESC msaaDepthBufferDesc{};
	msaaDepthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	msaaDepthBufferDesc.Alignment = 0;
	msaaDepthBufferDesc.Width = width;
	msaaDepthBufferDesc.Height = height;
	msaaDepthBufferDesc.DepthOrArraySize = 1;
	msaaDepthBufferDesc.MipLevels = 1;
	msaaDepthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
	msaaDepthBufferDesc.SampleDesc.Count = this->qualityLevels.SampleCount;
	msaaDepthBufferDesc.SampleDesc.Quality = this->qualityLevels.NumQualityLevels - 1;
	msaaDepthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	msaaDepthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_RESOURCE_DESC depthBufferDesc{};
	depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthBufferDesc.Alignment = 0;
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.DepthOrArraySize = 1;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		auto frame = (SwapFrame*)this->frameArray[i].Get();
		
		frame->msaaRenderTarget = nullptr;
		frame->msaaDepthBuffer = nullptr;
		frame->depthBuffer = nullptr;

		if (this->qualityLevels.SampleCount > 1 && this->qualityLevels.NumQualityLevels > 0)
		{
			result = device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&msaaRenderTargetDesc,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				&renderTargetClearValue,
				IID_PPV_ARGS(&frame->msaaRenderTarget));
			if (FAILED(result))
			{
				THEBE_LOG("Failed to create MSAA render target for frame %d.", i);
				return false;
			}

			result = device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&msaaDepthBufferDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthClearValue,
				IID_PPV_ARGS(&frame->msaaDepthBuffer));
			if (FAILED(result))
			{
				THEBE_LOG("Failed to create MSAA depth buffer for frame %d.", i);
				return false;
			}
		}

		result = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthBufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue,
			IID_PPV_ARGS(&frame->depthBuffer));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create depth buffer for frame %d.", i);
			return false;
		}
	}

	return true;
}

const CD3DX12_VIEWPORT& SwapChain::GetViewport() const
{
	return this->viewport;
}

IDXGISwapChain3* SwapChain::GetSwapChain()
{
	return this->swapChain.Get();
}

bool SwapChain::Resize(int width, int height)
{
	if (width == 0 || height == 0)
	{
		if (!this->GetWindowDimensions(width, height))
		{
			THEBE_LOG("Failed to get window dimensions.");
			return false;
		}
	}

	if (width == viewport.Width && height == viewport.Height)
		return true;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	graphicsEngine->WaitForGPUIdle();

	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		auto frame = (SwapFrame*)this->frameArray[i].Get();
		frame->renderTarget = nullptr;
	}

	HRESULT result = this->swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to resize swap chain.  Error: 0x%08x", result);
		return false;
	}

	if (!this->ResizeBuffers(width, height, graphicsEngine->GetDevice()))
		return false;

	if (!this->RecreateViews(graphicsEngine->GetDevice()))
		return false;

	this->viewport.TopLeftX = 0;
	this->viewport.TopLeftY = 0;
	this->viewport.Width = width;
	this->viewport.Height = height;

	this->scissorRect.left = 0;
	this->scissorRect.right = width;
	this->scissorRect.top = 0;
	this->scissorRect.bottom = height;

	THEBE_LOG("Swap-chain resized: %d x %d", width, height);

	return true;
}

bool SwapChain::CanAndShouldDoMSAA(SwapFrame* swapFrame)
{
	if (!this->msaaEnabled)
		return false;

	if (!swapFrame->msaaRenderTarget.Get())
		return false;

	if (!swapFrame->msaaDepthBuffer.Get())
		return false;

	return true;
}

bool SwapChain::GetWindowDimensions(int& width, int& height)
{
	RECT rect{};
	if (!GetClientRect(this->windowHandle, &rect))
		return false;

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	return true;
}

/*virtual*/ void SwapChain::PreSignal()
{
	// This must be done before a signal is enqueued, probably because this also uses the queue.
	// Note that calling this will change the return value of GetCurrentBackBufferIndex().
	HRESULT result = this->swapChain->Present(1, 0);
	THEBE_ASSERT(SUCCEEDED(result));
}

/*virtual*/ bool SwapChain::PreRender(ID3D12GraphicsCommandList* commandList, RenderObject::RenderContext& context)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	THEBE_ASSERT(graphicsEngine->GetFrameIndex() == this->swapChain->GetCurrentBackBufferIndex());

	context.camera = graphicsEngine->GetCameraSystem()->GetCamera();
	context.light = graphicsEngine->GetLight();

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (SwapFrame*)this->frameArray[frameIndex].Get();

	ID3D12Device* device = graphicsEngine->GetDevice();

	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	if (this->CanAndShouldDoMSAA(frame))
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->msaaRenderTarget.Get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrier);

		barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST);
		commandList->ResourceBarrier(1, &barrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvMsaaHandle;
		this->rtvMsaaDescriptorSet.GetCpuHandle(frameIndex, rtvMsaaHandle);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvMsaaHandle;
		this->dsvMsaaDescriptorSet.GetCpuHandle(frameIndex, dsvMsaaHandle);

		commandList->ClearRenderTargetView(rtvMsaaHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvMsaaHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->OMSetRenderTargets(1, &rtvMsaaHandle, FALSE, &dsvMsaaHandle);
	}
	else
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
		this->rtvDescriptorSet.GetCpuHandle(frameIndex, rtvHandle);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
		this->dsvDescriptorSet.GetCpuHandle(frameIndex, dsvHandle);

		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	}

	commandList->RSSetViewports(1, &this->viewport);
	commandList->RSSetScissorRects(1, &this->scissorRect);

	graphicsEngine->GetImGuiManager()->BeginRender();

	return true;
}

/*virtual*/ bool SwapChain::PostRender(ID3D12GraphicsCommandList* commandList)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (SwapFrame*)this->frameArray[frameIndex].Get();

	if (this->CanAndShouldDoMSAA(frame))
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->msaaRenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
		commandList->ResourceBarrier(1, &barrier);

		commandList->ResolveSubresource(frame->renderTarget.Get(), 0, frame->msaaRenderTarget.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);

		barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->renderTarget.Get(), D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
		this->rtvDescriptorSet.GetCpuHandle(frameIndex, rtvHandle);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
		this->dsvDescriptorSet.GetCpuHandle(frameIndex, dsvHandle);

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		graphicsEngine->GetImGuiManager()->EndRender(commandList);

		barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);
	}
	else
	{
		graphicsEngine->GetImGuiManager()->EndRender(commandList);

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);
	}

	return true;
}

/*virtual*/ void SwapChain::ConfigurePiplineStateDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (SwapFrame*)this->frameArray[frameIndex].Get();

	if (this->CanAndShouldDoMSAA(frame))
	{
		pipelineStateDesc.SampleDesc.Count = this->qualityLevels.SampleCount;
		pipelineStateDesc.SampleDesc.Quality = this->qualityLevels.NumQualityLevels - 1;
	}
	else
	{
		pipelineStateDesc.SampleDesc.Count = 1;
		pipelineStateDesc.SampleDesc.Quality = 0;
	}
}

//------------------------------------------------ SwapChain::SwapFrame ------------------------------------------------

SwapChain::SwapFrame::SwapFrame()
{
}

/*virtual*/ SwapChain::SwapFrame::~SwapFrame()
{
}