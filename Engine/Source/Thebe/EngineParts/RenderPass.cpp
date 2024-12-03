#include "Thebe/EngineParts/RenderPass.h"
#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/Camera.h"
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

	if (this->input)
	{
		this->input->Shutdown();
		this->input = nullptr;
	}

	if (this->output)
	{
		this->output->Shutdown();
		this->output = nullptr;
	}

	if (this->camera)
	{
		this->camera->Shutdown();
		this->camera = nullptr;
	}
}

/*virtual*/ bool RenderPass::Perform()
{
	if (!this->output.Get())
		return false;

	// Note that this call should stall if necessary until the returned command allocator can safely be reset.
	// A command allocator cannot be reset while the GPU is still executing commands that were allocated with it.
	ID3D12CommandAllocator* commandAllocator = this->output->AcquireCommandAllocator(this->commandQueue.Get());
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

	if (!this->output->PreRender(this->commandList.Get()))
	{
		THEBE_LOG("Pre-render failed.");
		return false;
	}
	
	if (this->input.Get())
	{
		ID3D12DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap()->GetDescriptorHeap();
		commandList->SetDescriptorHeaps(1, &csuDescriptorHeap);

		if (!this->input->Render(this->commandList.Get(), this->camera.Get()))
		{
			THEBE_LOG("Render failed.");
			return false;
		}
	}

	if (!this->output->PostRender(this->commandList.Get()))
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
	this->output->ReleaseCommandAllocator(commandAllocator, this->commandQueue.Get());

	return true;
}

RenderObject* RenderPass::GetInput()
{
	return this->input.Get();
}

RenderTarget* RenderPass::GetOutput()
{
	return this->output.Get();
}

Camera* RenderPass::GetCamera()
{
	return this->camera;
}

void RenderPass::SetInput(RenderObject* renderObject)
{
	this->input = renderObject;
}

void RenderPass::SetOutput(RenderTarget* renderTarget)
{
	this->output = renderTarget;
}

void RenderPass::SetCamera(Camera* camera)
{
	this->camera = camera;
}