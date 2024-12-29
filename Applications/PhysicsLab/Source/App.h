#pragma once

#include "Thebe/Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/EngineParts/PhysicsObject.h"
#include "JediCam.h"

class PhysicsLabApp : public Thebe::Application
{
public:
	PhysicsLabApp();
	virtual ~PhysicsLabApp();

	virtual bool PrepareForWindowShow() override;
	virtual void Shutdown(HINSTANCE instance) override;
	virtual LRESULT OnPaint(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT OnSize(WPARAM wParam, LPARAM lParam) override;
	virtual const char* GetWindowTitle() override;

private:
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::PerspectiveCamera> camera;
	Thebe::Reference<Thebe::DynamicLineRenderer> lineRenderer;
	Thebe::Reference<Thebe::PhysicsObject> objectA;
	Thebe::Reference<Thebe::PhysicsObject> objectB;
	JediCam jediCam;
};