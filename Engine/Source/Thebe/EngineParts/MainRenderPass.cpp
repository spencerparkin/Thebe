#include "Thebe/EngineParts/MainRenderPass.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/Log.h"

using namespace Thebe;

MainRenderPass::MainRenderPass()
{
}

/*virtual*/ MainRenderPass::~MainRenderPass()
{
}

/*virtual*/ bool MainRenderPass::Setup(void* data)
{
	if (!RenderPass::Setup(nullptr))
		return false;

	HWND windowHandle = (HWND)data;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	Reference<SwapChain> swapChain = new SwapChain();
	swapChain->SetGraphicsEngine(graphicsEngine.Get());
	SwapChain::SetupData swapChainSetupData{ windowHandle, this };
	if (!swapChain->Setup(&swapChainSetupData))
	{
		THEBE_LOG("Failed to setup the swap-chain for the main render pass.");
		return false;
	}

	this->output = swapChain.Get();

	return true;
}

/*virtual*/ void MainRenderPass::Shutdown()
{
	RenderPass::Shutdown();
}