#pragma once

#include "Thebe/Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/EngineParts/Text.h"
#include "Thebe/CollisionSystem.h"
#include "MoverCam.h"

class CollisionLabApp : public Thebe::Application
{
public:
	CollisionLabApp();
	virtual ~CollisionLabApp();

	virtual bool PrepareForWindowShow() override;
	virtual void Shutdown(HINSTANCE instance) override;
	virtual LRESULT OnPaint(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT OnSize(WPARAM wParam, LPARAM lParam) override;
	virtual const char* GetWindowTitle() override;

private:
	void RenderContacts(Thebe::CollisionSystem::Collision* collision, Thebe::DynamicLineRenderer* lineRenderer);

	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::PerspectiveCamera> camera;
	Thebe::Reference<Thebe::DynamicLineRenderer> lineRenderer;
	Thebe::Reference<Thebe::CollisionObject> shapeA;
	Thebe::Reference<Thebe::CollisionObject> shapeB;
	Thebe::Reference<Thebe::Text> text;
	MoverCam moverCam;
};