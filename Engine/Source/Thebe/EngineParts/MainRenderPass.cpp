#include "Thebe/EngineParts/MainRenderPass.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

MainRenderPass::MainRenderPass()
{
	this->windowHandle = NULL;
}

/*virtual*/ MainRenderPass::~MainRenderPass()
{
}

void MainRenderPass::SetWindowHandle(HWND windowHandle)
{
	this->windowHandle = windowHandle;
}

/*virtual*/ bool MainRenderPass::Setup()
{
	if (!this->windowHandle)
	{
		THEBE_LOG("No window handle configured.");
		return false;
	}

	if (!RenderPass::Setup())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	Reference<SwapChain> swapChain = new SwapChain();
	swapChain->SetGraphicsEngine(graphicsEngine.Get());
	swapChain->SetWindowHandle(this->windowHandle);
	swapChain->SetCommandQueue(this->commandQueue.Get());
	if (!swapChain->Setup())
	{
		THEBE_LOG("Failed to setup the swap-chain for the main render pass.");
		return false;
	}

	this->SetRenderTarget(swapChain.Get());
	return true;
}

/*virtual*/ void MainRenderPass::Shutdown()
{
	RenderPass::Shutdown();
}

/*virtual*/ bool MainRenderPass::GetRenderContext(RenderObject::RenderContext& context)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	context.camera = graphicsEngine->GetCamera();
	context.light = graphicsEngine->GetLight();
	return true;
}