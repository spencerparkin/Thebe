#include "TestApplication.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/MeshInstance.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Buffer.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/DirectionalLight.h"
#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/Math/Transform.h"

using namespace Thebe;

TestApplication::TestApplication()
{
}

/*virtual*/ TestApplication::~TestApplication()
{
}

/*virtual*/ bool TestApplication::PrepareForWindowShow()
{
	this->graphicsEngine.Set(new GraphicsEngine());

	// Tell the engine where it can find assets.  We're just using what assets the engine comes with by default.
	if (!this->graphicsEngine->AddAssetFolder("Engine/Assets"))
		return false;

	// Initialize the engine.
	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

	// Load an env-map.  Must do this before loading the scene.
	Reference<CubeMapBuffer> envMap;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Textures/OceanCubeMap/OceanCubeMap.cube_map)", envMap))
		return false;
	this->graphicsEngine->SetEnvMap(envMap);

	// Load and configure the scene.
	Reference<Scene> scene;
	if (!this->graphicsEngine->LoadEnginePartFromFile(R"(Scenes\Silly.scene)", scene))
		return false;
	this->graphicsEngine->SetRenderObject(scene);

	// Add framerate indicator.
	this->framerateText.Set(new FramerateText());
	this->framerateText->SetGraphicsEngine(this->graphicsEngine);
	this->framerateText->SetFlags(0);
	if (!this->framerateText->Setup())
		return false;
	scene->GetRootSpace()->AddSubSpace(this->framerateText);

	// Add a line renderer.
	this->lineRenderer.Set(new DynamicLineRenderer());
	this->lineRenderer->SetGraphicsEngine(this->graphicsEngine);
	this->lineRenderer->SetLineMaxCount(128);
	this->lineRenderer->SetLineRenderCount(this->lineRenderer->GetLineMaxCount());
	if (!this->lineRenderer->Setup())
		return false;
	scene->GetRenderObjectArray().push_back(this->lineRenderer.Get());

	// Add some lines to the line renderer.
	double radius = 10.0;
	std::vector<Vector3> pointArray;
	std::vector<Vector3> colorArray;
	for (UINT i = 0; i < this->lineRenderer->GetLineRenderCount(); i++)
	{
		double angle = 2.0 * M_PI * double(i) / double(this->lineRenderer->GetLineRenderCount());
		Vector3 point(radius * cos(angle), 50.0, radius * sin(angle));
		pointArray.push_back(point);
		Vector3 color = ((i % 2) == 0) ? Vector3(1.0, 1.0, 1.0) : Vector3(0.0, 0.0, 0.0);
		colorArray.push_back(color);
	}
	for (UINT i = 0; i < this->lineRenderer->GetLineRenderCount(); i++)
	{
		const Vector3& pointA = pointArray[i];
		const Vector3& pointB = pointArray[(i + 1) % this->lineRenderer->GetLineRenderCount()];
		const Vector3& colorA = colorArray[i];
		const Vector3& colorB = colorArray[(i + 1) % this->lineRenderer->GetLineRenderCount()];
		this->lineRenderer->SetLine(i, pointA, pointB, &colorA, &colorB);
	}

	// Load and configure a light source.
	Reference<DirectionalLight> light(new DirectionalLight());
	light->Setup();
	Transform lightToWorld;
	lightToWorld.LookAt(Vector3(50.0, 100.0, 50.0), Vector3(0.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0));
	light->SetLightToWorldTransform(lightToWorld);
	this->graphicsEngine->SetLight(light);

	// Configure a camera.
	Transform cameraToWorld;
	cameraToWorld.matrix.SetIdentity();
	cameraToWorld.translation.SetComponents(0.0, 100.0, 50.0);
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);

	// Let the our free-cam control the camera.
	this->freeCam.SetCamera(this->camera);

	return true;
}

/*virtual*/ void TestApplication::Shutdown(HINSTANCE instance)
{
	if (this->graphicsEngine.Get())
	{
		this->graphicsEngine->Shutdown();
		this->graphicsEngine = nullptr;
	}

	Application::Shutdown(instance);
}

/*virtual*/ LRESULT TestApplication::OnPaint(WPARAM wParam, LPARAM lParam)
{
	this->graphicsEngine->Render();
	
	this->freeCam.Update(this->graphicsEngine->GetDeltaTime());

	XBoxController* controller = this->freeCam.GetController();
	if (controller->WasButtonPressed(XINPUT_GAMEPAD_X))
	{
		if (this->graphicsEngine->GetCamera() != this->camera.Get())
			this->graphicsEngine->SetCamera(this->camera.Get());
		else
		{
			// View the scene from the perspective of the light.
			Camera* lightCamera = this->graphicsEngine->GetLight()->GetCamera();
			this->graphicsEngine->SetCamera(lightCamera);
		}
	}

	if (controller->WasButtonPressed(XINPUT_GAMEPAD_Y))
	{
		auto swapChain = this->graphicsEngine->FindRenderTarget<Thebe::SwapChain>();
		if (swapChain)
			swapChain->SetMsaaEnabled(!swapChain->GetMsaaEnabled());
	}

	if (controller->WasButtonPressed(XINPUT_GAMEPAD_B))
	{
		uint32_t flags = this->framerateText->GetFlags();
		if ((flags & THEBE_RENDER_OBJECT_FLAG_VISIBLE) != 0)
			flags &= ~THEBE_RENDER_OBJECT_FLAG_VISIBLE;
		else
			flags |= THEBE_RENDER_OBJECT_FLAG_VISIBLE;
		this->framerateText->SetFlags(flags);
	}

	return 0;
}

/*virtual*/ LRESULT TestApplication::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}

/*virtual*/ void TestApplication::BetweenDispatches()
{
}