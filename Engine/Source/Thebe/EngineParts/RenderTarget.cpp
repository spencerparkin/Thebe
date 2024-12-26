#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

//---------------------------------------- RenderTarget ----------------------------------------

RenderTarget::RenderTarget()
{
}

/*virtual*/ RenderTarget::~RenderTarget()
{
}

/*virtual*/ bool RenderTarget::Setup()
{
	if (this->frameArray.size() > 0)
	{
		THEBE_LOG("Render target already setup.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();

	for (int i = 0; i < THEBE_NUM_SWAP_FRAMES; i++)
	{
		Reference<Frame> frame = this->NewFrame();
		if (!frame.Get())
		{
			THEBE_LOG("Failed to create frame for render target %d.", i);
			return false;
		}

		frame->SetGraphicsEngine(graphicsEngine.Get());
		frame->SetRenderTargetOwner(this);
		frame->SetFrameNumber(i);
		if (!frame->Setup())
		{
			THEBE_LOG("Failed to setup frame %d for render target.", i);
			return false;
		}

		this->frameArray.push_back(frame);
	}

	return true;
}

/*virtual*/ void RenderTarget::Shutdown()
{
	for (Reference<Frame>& frame : this->frameArray)
	{
		frame->Shutdown();
		frame = nullptr;
	}

	this->frameArray.clear();

	EnginePart::Shutdown();
}

/*virtual*/ bool RenderTarget::PreRender(ID3D12GraphicsCommandList* commandList, RenderObject::RenderContext& context)
{
	return false;
}

/*virtual*/ bool RenderTarget::PostRender(ID3D12GraphicsCommandList* commandList)
{
	return false;
}

/*virtual*/ bool RenderTarget::Render()
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UINT frameIndex = graphicsEngine->GetFrameIndex();
	THEBE_ASSERT(0 <= frameIndex && frameIndex < (UINT)this->frameArray.size());
	Frame* frame = this->frameArray[frameIndex];

	if (!frame->BeginRecordingCommandList())
	{
		THEBE_LOG("Failed to begin command-list recording.");
		return false;
	}

	bool renderSucceeded = false;
	do
	{
		ID3D12GraphicsCommandList* commandList = frame->GetCommandList();
		if (!commandList)
			break;

		RenderObject::RenderContext context{};
		context.renderTarget = this;
		if (!this->PreRender(commandList, context))
		{
			THEBE_LOG("Pre-render failed.");
			break;
		}

		RenderObject* renderObject = graphicsEngine->GetRenderObject();
		if (renderObject)
		{
			ID3D12DescriptorHeap* csuDescriptorHeap = graphicsEngine->GetCSUDescriptorHeap()->GetDescriptorHeap();
			commandList->SetDescriptorHeaps(1, &csuDescriptorHeap);

			if (!renderObject->Render(commandList, &context))
			{
				THEBE_LOG("Render failed.");
				break;
			}
		}

		if (!this->PostRender(commandList))
		{
			THEBE_LOG("Post-render failed.");
			break;
		}

		renderSucceeded = true;
	} while (false);

	if (!renderSucceeded)
	{
		if (!frame->CancelRecordingCommandList())
			THEBE_LOG("Failed to cancel command-list rendering.");
		
		return false;
	}

	if (!frame->EndRecordingCommandList())
	{
		THEBE_LOG("Failed to end command-list recording.");
		return false;
	}

	return true;
}

/*virtual*/ void RenderTarget::PreSignal()
{
}

/*virtual*/ void RenderTarget::ConfigurePiplineStateDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc)
{
}

//---------------------------------------- RenderTarget::Frame ----------------------------------------

RenderTarget::Frame::Frame()
{
	this->renderTargetOwner = nullptr;
	this->frameNumber = -1;
}

/*virtual*/ RenderTarget::Frame::~Frame()
{
}

void RenderTarget::Frame::SetRenderTargetOwner(RenderTarget* renderTargetOwner)
{
	this->renderTargetOwner = renderTargetOwner;
}

void RenderTarget::Frame::SetFrameNumber(UINT frameNumber)
{
	this->frameNumber = frameNumber;
}

/*virtual*/ void RenderTarget::Frame::PreSignal()
{
	if (this->renderTargetOwner)
		this->renderTargetOwner->PreSignal();
}