#include "Thebe/EngineParts/RenderPass.h"
#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

RenderPass::RenderPass()
{
}

/*virtual*/ RenderPass::~RenderPass()
{
}

/*virtual*/ bool RenderPass::Setup()
{
	if (!CommandQueue::Setup())
		return false;

	return true;
}

/*virtual*/ void RenderPass::Shutdown()
{
	CommandQueue::Shutdown();

	if (this->renderTarget)
	{
		this->renderTarget->Shutdown();
		this->renderTarget = nullptr;
	}
}

/*virtual*/ bool RenderPass::Render()
{
	if (!this->renderTarget.Get())
		return false;

	// Note that this call should stall if necessary until the returned command allocator can safely be reset.
	// A command allocator cannot be reset while the GPU is still executing commands that were allocated with it.
	ID3D12CommandAllocator* commandAllocator = this->renderTarget->AcquireCommandAllocator(this->commandQueue.Get());
	if (!commandAllocator)
	{
		THEBE_LOG("No command allocator given by the render pass output.");
		return false;
	}

	HRESULT result = commandAllocator->Reset();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to reset command allocator.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	if (!this->commandList.Get())
	{
		result = graphicsEngine->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&this->commandList));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create command list.  Error: 0x%08x", result);
			return false;
		}

		//this->commandList->SetName(L"");
	}
	else
	{
		result = this->commandList->Reset(commandAllocator, nullptr);
		if (FAILED(result))
		{
			THEBE_LOG("Failed to reset command list and put it into the recording state.");
			return false;
		}
	}

	if (!this->renderTarget->PreRender(this->commandList.Get()))
	{
		THEBE_LOG("Pre-render failed.");
		return false;
	}
	
	RenderObject* renderObject = this->GetRenderObject();
	RenderObject::RenderContext context{};
	if (renderObject && this->GetRenderContext(context))
	{
		ID3D12DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap()->GetDescriptorHeap();
		commandList->SetDescriptorHeaps(1, &csuDescriptorHeap);

		if (!renderObject->Render(this->commandList.Get(), &context))
		{
			THEBE_LOG("Render failed.");
			return false;
		}
	}

	if (!this->renderTarget->PostRender(this->commandList.Get()))
	{
		THEBE_LOG("Post-render failed.");
		return false;
	}

	result = this->commandList->Close();
	if (FAILED(result))
	{
		THEBE_LOG("Failed to close command list.");
		return false;
	}

	// Kick-off the GPU to execute the commands.
	ID3D12CommandList* commandListArray[] = { this->commandList.Get() };
	this->commandQueue->ExecuteCommandLists(_countof(commandListArray), commandListArray);

	// Indicate that we no longer need the command allocator.
	this->renderTarget->ReleaseCommandAllocator(commandAllocator, this->commandQueue.Get());

	return true;
}

/*virtual*/ bool RenderPass::GetRenderContext(RenderObject::RenderContext& context)
{
	return false;
}

/*virtual*/ RenderObject* RenderPass::GetRenderObject()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return nullptr;

	return graphicsEngine->GetRenderObject();
}

RenderTarget* RenderPass::GetRenderTarget()
{
	return this->renderTarget;
}

void RenderPass::SetRenderTarget(RenderTarget* renderTarget)
{
	this->renderTarget = renderTarget;
}