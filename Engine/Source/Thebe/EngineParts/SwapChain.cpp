#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

SwapChain::SwapChain()
{
	this->windowHandle = NULL;
	this->SetName("SwapChain");
}

/*virtual*/ SwapChain::~SwapChain()
{
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

	ComPtr<IDXGISwapChain1> swapChain1;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = THEBE_NUM_SWAP_FRAMES;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	HRESULT result = factory->CreateSwapChainForHwnd(
		this->commandQueue.Get(),
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

	if (!this->ResizeDepthBuffers(width, height, device))
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
	}

	for (int i = 0; i < (int)this->frameArray.size(); i++)
	{
		auto frame = (SwapFrame*)this->frameArray[i];
		frame->depthBuffer = nullptr;
		frame->renderTarget = nullptr;
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
		auto frame = (SwapFrame*)this->frameArray[i];

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
	}

	return true;
}

bool SwapChain::ResizeDepthBuffers(int width, int height, ID3D12Device* device)
{
	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		auto frame = (SwapFrame*)this->frameArray[i];
		
		frame->depthBuffer = nullptr;

		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

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

		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		HRESULT result = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthBufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&frame->depthBuffer));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create depth buffer %d.", i);
			return false;
		}
	}

	return true;
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
		auto frame = (SwapFrame*)this->frameArray[i];
		frame->renderTarget = nullptr;
	}

	HRESULT result = this->swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to resize swap chain.  Error: 0x%08x", result);
		return false;
	}

	if (!this->ResizeDepthBuffers(width, height, graphicsEngine->GetDevice()))
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

/*virtual*/ bool SwapChain::PreRender(RenderObject::RenderContext& context)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	THEBE_ASSERT(graphicsEngine->GetFrameIndex() == this->swapChain->GetCurrentBackBufferIndex());

	context.camera = graphicsEngine->GetCamera();
	context.light = graphicsEngine->GetLight();

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (SwapFrame*)this->frameArray[frameIndex];

	ID3D12Device* device = graphicsEngine->GetDevice();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	this->commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	this->rtvDescriptorSet.GetCpuHandle(frameIndex, rtvHandle);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	this->dsvDescriptorSet.GetCpuHandle(frameIndex, dsvHandle);

	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	this->commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	this->commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	this->commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	this->commandList->RSSetViewports(1, &this->viewport);
	this->commandList->RSSetScissorRects(1, &this->scissorRect);

	return true;
}

/*virtual*/ bool SwapChain::PostRender()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (SwapFrame*)this->frameArray[frameIndex];

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	this->commandList->ResourceBarrier(1, &barrier);

	return true;
}