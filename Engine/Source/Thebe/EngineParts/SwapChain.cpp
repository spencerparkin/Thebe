#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/RenderPass.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

SwapChain::SwapChain()
{
	this->windowHandle = NULL;
}

/*virtual*/ SwapChain::~SwapChain()
{
}

/*virtual*/ bool SwapChain::Setup(void* data)
{
	if (this->windowHandle)
		return false;

	auto setupData = static_cast<SetupData*>(data);
	this->windowHandle = setupData->windowHandle;

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
		setupData->renderPass->GetCommandQueue(),
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

	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		Frame& frame = this->frameArray[i];
		frame.fence = new Fence();
		frame.fence->SetGraphicsEngine(graphicsEngine.Get());
		if (!frame.fence->Setup(nullptr))
			return false;

		result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.commandAllocator));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create command allocator for swap-frame %d.  Error: 0x%08x", result);
			return false;
		}
	}

	if (!this->RecreateRenderTargetViews())
	{
		THEBE_LOG("Failed to create render target views into each swap-frame.");
		return false;
	}

	return true;
}

/*virtual*/ void SwapChain::Shutdown()
{
	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		Frame& frame = this->frameArray[i];
		frame.fence->Shutdown();
		frame.fence = nullptr;
		frame.renderTarget = nullptr;
		frame.commandAllocator = nullptr;
	}

	this->rtvHeap = nullptr;
}

bool SwapChain::RecreateRenderTargetViews()
{
	this->rtvHeap = nullptr;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = THEBE_NUM_SWAP_FRAMES;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT result = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&this->rtvHeap));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create RTV descriptor heap.  Error: 0x%08x", result);
		return false;
	}

	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(this->rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		Frame& frame = this->frameArray[i];

		result = this->swapChain->GetBuffer(i, IID_PPV_ARGS(&frame.renderTarget));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to get render target for frame %d.  Error: 0x%08x", i, result);
			return false;
		}

		wchar_t renderTargetName[512];
		wsprintfW(renderTargetName, L"Swap Frame Render Target %d", i);
		frame.renderTarget->SetName(renderTargetName);

		device->CreateRenderTargetView(frame.renderTarget.Get(), nullptr, rtvHandle);

		rtvHandle.Offset(1, rtvDescriptorSize);
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
		Frame& frame = this->frameArray[i];
		frame.renderTarget = nullptr;
	}

	HRESULT result = this->swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result))
	{
		THEBE_LOG("Failed to resize swap chain.  Error: 0x%08x", result);
		return false;
	}

	if (!this->RecreateRenderTargetViews())
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

/*virtual*/ ID3D12CommandAllocator* SwapChain::AcquireCommandAllocator(ID3D12CommandQueue* commandQueue)
{
	UINT i = this->swapChain->GetCurrentBackBufferIndex();
	THEBE_ASSERT(0 <= i && i < THEBE_NUM_SWAP_FRAMES);
	Frame& frame = this->frameArray[i];

	// Make sure the GPU is done rendering this part of the swap-chain before we return the command allocator.
	frame.fence->WaitForSignalIfNecessary();

	return frame.commandAllocator.Get();
}

/*virtual*/ void SwapChain::ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue)
{
	// Call this before we present.  Otherwise, the back-buffer index will change on us.
	UINT i = this->swapChain->GetCurrentBackBufferIndex();
	THEBE_ASSERT(0 <= i && i < THEBE_NUM_SWAP_FRAMES);
	Frame& frame = this->frameArray[i];

	// Internally, I wonder if this enqueues some commands on the command list.  When the swap-chain was
	// created, we had to pass the creation function a pointer to the command queue.
	HRESULT result = this->swapChain->Present(1, 0);

	// By this time, any command lists submitted to the GPU have been enqueued.
	// The last thing to enqueue is a signal we can use to know that the frame is complete.
	frame.fence->EnqueueSignal(commandQueue);
}

/*virtual*/ bool SwapChain::PreRender(ID3D12GraphicsCommandList* commandList)
{
	UINT i = this->swapChain->GetCurrentBackBufferIndex();
	THEBE_ASSERT(0 <= i && i < THEBE_NUM_SWAP_FRAMES);
	Frame& frame = this->frameArray[i];

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &barrier);

	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(this->rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, rtvDescriptorSize);

	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	commandList->RSSetViewports(1, &this->viewport);
	commandList->RSSetScissorRects(1, &this->scissorRect);

	return true;
}

/*virtual*/ bool SwapChain::PostRender(ID3D12GraphicsCommandList* commandList)
{
	UINT i = this->swapChain->GetCurrentBackBufferIndex();
	THEBE_ASSERT(0 <= i && i < THEBE_NUM_SWAP_FRAMES);
	Frame& frame = this->frameArray[i];

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrier);

	return true;
}

int SwapChain::GetCurrentBackBufferIndex()
{
	return this->swapChain->GetCurrentBackBufferIndex();
}