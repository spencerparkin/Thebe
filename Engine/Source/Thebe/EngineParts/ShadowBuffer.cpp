#include "Thebe/EngineParts/ShadowBuffer.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

ShadowBuffer::ShadowBuffer()
{
	this->currentFrame = 0;
}

/*virtual*/ ShadowBuffer::~ShadowBuffer()
{
}

/*virtual*/ bool ShadowBuffer::Setup()
{
	if (!RenderTarget::Setup())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	HRESULT result = 0;

	if (!graphicsEngine->GetDSVDescriptorHeap()->AllocDescriptorSet(THEBE_NUM_SWAP_FRAMES, this->dsvDescriptorSet))
	{
		THEBE_LOG("Failed to allocate DSV descriptor set for shadow buffer.");
		return false;
	}

	this->viewport.TopLeftX = 0;
	this->viewport.TopLeftY = 0;
	this->viewport.Width = THEBE_SHADOW_BUFFER_WIDTH;
	this->viewport.Height = THEBE_SHADOW_BUFFER_HEIGHT;
	this->viewport.MinDepth = 0.0;
	this->viewport.MaxDepth = 1.0;

	this->scissorRect.left = 0;
	this->scissorRect.right = THEBE_SHADOW_BUFFER_WIDTH;
	this->scissorRect.top = 0;
	this->scissorRect.bottom = THEBE_SHADOW_BUFFER_HEIGHT;

	if (this->frameArray.size() != THEBE_NUM_SWAP_FRAMES)
		return false;

	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		auto frame = (ShadowFrame*)this->frameArray[i];

		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RESOURCE_DESC depthBufferDesc{};
		depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthBufferDesc.Alignment = 0;
		depthBufferDesc.Width = THEBE_SHADOW_BUFFER_WIDTH;
		depthBufferDesc.Height = THEBE_SHADOW_BUFFER_HEIGHT;
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
			D3D12_RESOURCE_STATE_DEPTH_READ,
			&clearValue,
			IID_PPV_ARGS(&frame->depthBuffer));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create depth buffer %d.", i);
			return false;
		}

		wchar_t shadowBufferName[128];
		wsprintfW(shadowBufferName, L"Shadow Buffer Target %d", i);
		frame->depthBuffer->SetName(shadowBufferName);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
		this->dsvDescriptorSet.GetCpuHandle(i, dsvHandle);
		device->CreateDepthStencilView(frame->depthBuffer.Get(), nullptr, dsvHandle);
	}

	return true;
}

/*virtual*/ void ShadowBuffer::Shutdown()
{
	for (int i = 0; i < (int)this->frameArray.size(); i++)
	{
		auto frame = (ShadowFrame*)frameArray[i];
		frame->depthBuffer = nullptr;
	}

	RenderTarget::Shutdown();
}

/*virtual*/ ID3D12CommandAllocator* ShadowBuffer::AcquireCommandAllocator(ID3D12CommandQueue* commandQueue)
{
	this->currentFrame = (this->currentFrame + 1) % THEBE_NUM_SWAP_FRAMES;

	return RenderTarget::AcquireCommandAllocator(commandQueue);
}

/*virtual*/ void ShadowBuffer::ReleaseCommandAllocator(ID3D12CommandAllocator* commandAllocator, ID3D12CommandQueue* commandQueue)
{
	RenderTarget::ReleaseCommandAllocator(commandAllocator, commandQueue);
}

/*virtual*/ bool ShadowBuffer::PreRender(ID3D12GraphicsCommandList* commandList)
{
	auto frame = (ShadowFrame*)this->frameArray[this->currentFrame];

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->depthBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	this->dsvDescriptorSet.GetCpuHandle(this->currentFrame, dsvHandle);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

	commandList->RSSetViewports(1, &this->viewport);
	commandList->RSSetScissorRects(1, &this->scissorRect);

	return true;
}

/*virtual*/ bool ShadowBuffer::PostRender(ID3D12GraphicsCommandList* commandList)
{
	auto frame = (ShadowFrame*)this->frameArray[this->currentFrame];

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->depthBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
	commandList->ResourceBarrier(1, &barrier);

	return true;
}

/*virtual*/ RenderTarget::Frame* ShadowBuffer::NewFrame()
{
	return new ShadowFrame();
}

/*virtual*/ UINT ShadowBuffer::GetCurrentFrame()
{
	return this->currentFrame;
}