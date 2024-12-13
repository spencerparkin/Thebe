#include "Thebe/EngineParts/ShadowPass.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/ShadowBuffer.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

ShadowPass::ShadowPass()
{
}

/*virtual*/ ShadowPass::~ShadowPass()
{
}

/*virtual*/ bool ShadowPass::Setup()
{
	if (!RenderPass::Setup())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	Reference<ShadowBuffer> shadowBuffer = new ShadowBuffer();
	shadowBuffer->SetGraphicsEngine(graphicsEngine.Get());
	if (!shadowBuffer->Setup())
	{
		THEBE_LOG("Failed to setup shadow buffer for shadow pass.");
		return false;
	}

	this->SetRenderTarget(shadowBuffer.Get());
	return true;
}

/*virtual*/ void ShadowPass::Shutdown()
{
	RenderPass::Shutdown();
}

/*virtual*/ bool ShadowPass::GetRenderContext(RenderObject::RenderContext& context)
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

	context.light = nullptr;
	context.camera = camera;
	return true;
}