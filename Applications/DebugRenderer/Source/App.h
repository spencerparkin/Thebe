#pragma once

#include "Thebe/Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/DynamicLineRenderer.h"
#include "Thebe/Network/DebugRenderServer.h"

class DebugRendererApplication : public Thebe::Application
{
public:
	DebugRendererApplication();
	virtual ~DebugRendererApplication();

	virtual bool PrepareForWindowShow() override;
	virtual void Shutdown(HINSTANCE instance) override;
	virtual LRESULT OnPaint(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT OnSize(WPARAM wParam, LPARAM lParam) override;

private:
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::PerspectiveCamera> camera;
	Thebe::Reference<Thebe::XBoxController> controller;
	Thebe::Reference<Thebe::DynamicLineRenderer> lineRenderer;
	Thebe::DebugRenderServer server;
};