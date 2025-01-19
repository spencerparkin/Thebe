#include "App.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"

using namespace Thebe;

DebugRendererApplication::DebugRendererApplication()
{
}

/*virtual*/ DebugRendererApplication::~DebugRendererApplication()
{
}

/*virtual*/ bool DebugRendererApplication::PrepareForWindowShow()
{
	if (!this->server.Setup())
		return false;

	this->graphicsEngine.Set(new GraphicsEngine());
	if (!this->graphicsEngine->AddAssetFolder("Engine/Assets"))
		return false;

	if (!this->graphicsEngine->Setup(this->windowHandle))
		return false;

	Reference<Scene> scene(new Scene());
	this->graphicsEngine->SetRenderObject(scene);

	this->lineRenderer.Set(new DynamicLineRenderer());
	this->lineRenderer->SetGraphicsEngine(this->graphicsEngine);
	this->lineRenderer->SetLineMaxCount(128);
	if (!this->lineRenderer->Setup())
		return false;

	scene->GetRenderObjectArray().push_back(this->lineRenderer.Get());

	Transform cameraToWorld;
	cameraToWorld.LookAt(Vector3(10.0, 10.0, 10.0), Vector3::Zero(), Vector3(0.0, 1.0, 0.0));
	this->camera.Set(new PerspectiveCamera());
	this->camera->SetCameraToWorldTransform(cameraToWorld);
	this->graphicsEngine->SetCamera(this->camera);

	this->freeCam.SetCamera(this->camera);

	return true;
}

/*virtual*/ void DebugRendererApplication::Shutdown(HINSTANCE instance)
{
	this->server.Shutdown();

	if (this->graphicsEngine.Get())
	{
		this->graphicsEngine->Shutdown();
		this->graphicsEngine = nullptr;
	}

	Application::Shutdown(instance);
}

/*virtual*/ LRESULT DebugRendererApplication::OnPaint(WPARAM wParam, LPARAM lParam)
{
	this->server.Serve();

	this->lineRenderer->ResetLines();

	Vector3 xAxis = Vector3::XAxis();
	Vector3 yAxis = Vector3::YAxis();
	Vector3 zAxis = Vector3::ZAxis();

	this->lineRenderer->AddLine(Vector3::Zero(), xAxis, &xAxis, &xAxis);
	this->lineRenderer->AddLine(Vector3::Zero(), yAxis, &yAxis, &yAxis);
	this->lineRenderer->AddLine(Vector3::Zero(), zAxis, &zAxis, &zAxis);

	static bool hackA = false;
	if (hackA)
	{
		static Vector3 pointsA[2];
		static Vector3 colorA(1.0, 1.0, 1.0);
		this->lineRenderer->AddLine(pointsA[0], pointsA[1], &colorA, &colorA);
	}

	static bool hackB = false;
	if (hackB)
	{
		static Vector3 pointsB[2];
		static Vector3 colorB(1.0, 1.0, 1.0);
		this->lineRenderer->AddLine(pointsB[0], pointsB[1], &colorB, &colorB);
	}

	this->server.Draw(this->lineRenderer);

	this->graphicsEngine->Render();

	this->freeCam.Update(this->graphicsEngine->GetDeltaTime());

	XBoxController* controller = this->freeCam.GetController();
	if (controller->WasButtonPressed(XINPUT_GAMEPAD_B))
	{
		Transform cameraToWorld;
		cameraToWorld.LookAt(Vector3(10.0, 10.0, 10.0), Vector3::Zero(), Vector3(0.0, 1.0, 0.0));
		this->camera->SetCameraToWorldTransform(cameraToWorld);
	}
	else if (controller->WasButtonPressed(XINPUT_GAMEPAD_Y))
	{
		this->server.ClearAll();
	}

	return 0;
}

/*virtual*/ LRESULT DebugRendererApplication::OnSize(WPARAM wParam, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	this->graphicsEngine->Resize(width, height);

	return 0;
}