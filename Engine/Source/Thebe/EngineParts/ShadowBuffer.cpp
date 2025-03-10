#include "Thebe/EngineParts/ShadowBuffer.h"
#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------------ ShadowBuffer ------------------------------------

ShadowBuffer::ShadowBuffer()
{
	this->SetName("ShadowBuffer");
}

/*virtual*/ ShadowBuffer::~ShadowBuffer()
{
}

/*virtual*/ bool ShadowBuffer::Setup()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!graphicsEngine->GetDSVDescriptorHeap()->AllocDescriptorSet(THEBE_NUM_SWAP_FRAMES, this->dsvDescriptorSet))
	{
		THEBE_LOG("Failed to allocate DSV descriptor set for shadow buffer.");
		return false;
	}

	if (!RenderTarget::Setup())
		return false;

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

	return true;
}

/*virtual*/ void ShadowBuffer::Shutdown()
{
	Reference<GraphicsEngine> graphicsEngine;
	this->GetGraphicsEngine(graphicsEngine);

	if (graphicsEngine.Get())
	{
		if (this->dsvDescriptorSet.IsAllocated())
			graphicsEngine->GetDSVDescriptorHeap()->FreeDescriptorSet(this->dsvDescriptorSet);
	}

	RenderTarget::Shutdown();
}

/*virtual*/ bool ShadowBuffer::PreRender(ID3D12GraphicsCommandList* commandList, RenderObject::RenderContext& context)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	Light* light = graphicsEngine->GetLight();
	if (!light)
		return false;

	Camera* camera = light->GetCamera();
	if (!camera)
		return false;

	camera->UpdateProjection(1.0);

	context.light = nullptr;
	context.camera = camera;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (ShadowFrame*)this->frameArray[frameIndex].Get();

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	this->dsvDescriptorSet.GetCpuHandle(frameIndex, dsvHandle);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

	commandList->RSSetViewports(1, &this->viewport);
	commandList->RSSetScissorRects(1, &this->scissorRect);

	return true;
}

/*virtual*/ bool ShadowBuffer::PostRender(ID3D12GraphicsCommandList* commandList)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (ShadowFrame*)this->frameArray[frameIndex].Get();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->depthBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->ResourceBarrier(1, &barrier);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->depthTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->ResourceBarrier(1, &barrier);

	commandList->CopyResource(frame->depthTexture.Get(), frame->depthBuffer.Get());

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->depthBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->ResourceBarrier(1, &barrier);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(frame->depthTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	commandList->ResourceBarrier(1, &barrier);

	return true;
}

/*virtual*/ RenderTarget::Frame* ShadowBuffer::NewFrame()
{
	return new ShadowFrame();
}

DescriptorHeap::DescriptorSet* ShadowBuffer::GetShadowMapDescriptorForShader()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return nullptr;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	auto frame = (ShadowFrame*)this->frameArray[frameIndex].Get();
	return &frame->srvDescriptorSet;
}

/*virtual*/ void ShadowBuffer::ConfigurePiplineStateDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc)
{
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleDesc.Quality = 0;
}

//------------------------------------ ShadowBuffer::ShadowFrame ------------------------------------

ShadowBuffer::ShadowFrame::ShadowFrame()
{
}

/*virtual*/ ShadowBuffer::ShadowFrame::~ShadowFrame()
{
}

/*virtual*/ bool ShadowBuffer::ShadowFrame::Setup()
{
	if (!Frame::Setup())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();

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
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&this->depthBuffer));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create depth buffer for frame %d.", this->frameNumber);
		return false;
	}

	wchar_t shadowBufferName[128];
	wsprintfW(shadowBufferName, L"Shadow Buffer Target %d", this->frameNumber);
	this->depthBuffer->SetName(shadowBufferName);

	auto shadowBuffer = (ShadowBuffer*)this->renderTargetOwner;
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	shadowBuffer->dsvDescriptorSet.GetCpuHandle(this->frameNumber, dsvHandle);
	device->CreateDepthStencilView(this->depthBuffer.Get(), nullptr, dsvHandle);

	D3D12_RESOURCE_DESC depthTextureDesc{};
	depthTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthTextureDesc.Alignment = 0;
	depthTextureDesc.Width = THEBE_SHADOW_BUFFER_WIDTH;
	depthTextureDesc.Height = THEBE_SHADOW_BUFFER_HEIGHT;
	depthTextureDesc.DepthOrArraySize = 1;
	depthTextureDesc.MipLevels = 1;
	depthTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthTextureDesc.SampleDesc.Count = 1;
	depthTextureDesc.SampleDesc.Quality = 0;
	depthTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	result = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthTextureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&this->depthTexture));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create depth texture %d.", this->frameNumber);
		return false;
	}

	wchar_t shadowTextureName[128];
	wsprintfW(shadowTextureName, L"Shadow Texture %d", this->frameNumber);
	this->depthTexture->SetName(shadowTextureName);

	if (!graphicsEngine->GetCSUDescriptorHeap()->AllocDescriptorSet(1, this->srvDescriptorSet))
	{
		THEBE_LOG("Failed to allocate SRV descriptor set %d for shadow buffer.", this->frameNumber);
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = depthTextureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE csuHandle;
	this->srvDescriptorSet.GetCpuHandle(0, csuHandle);
	device->CreateShaderResourceView(this->depthTexture.Get(), &srvDesc, csuHandle);

	return true;
}

/*virtual*/ void ShadowBuffer::ShadowFrame::Shutdown()
{
	Frame::Shutdown();

	this->depthBuffer = nullptr;
	this->depthTexture = nullptr;

	Reference<GraphicsEngine> graphicsEngine;
	this->GetGraphicsEngine(graphicsEngine);
	if (graphicsEngine.Get())
	{
		if (this->srvDescriptorSet.IsAllocated())
			graphicsEngine->GetCSUDescriptorHeap()->FreeDescriptorSet(this->srvDescriptorSet);
	}
}