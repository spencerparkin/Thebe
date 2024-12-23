#pragma once

#include "Thebe/Application.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/TextInstance.h"
#include "Thebe/FreeCam.h"

class TestApplication : public Thebe::Application
{
public:
	TestApplication();
	virtual ~TestApplication();

	virtual bool PrepareForWindowShow() override;
	virtual void Shutdown(HINSTANCE instance) override;
	virtual LRESULT OnPaint(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT OnSize(WPARAM wParam, LPARAM lParam) override;

private:
	virtual void BetweenDispatches() override;

	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
	Thebe::Reference<Thebe::PerspectiveCamera> camera;
	Thebe::FreeCam freeCam;
	Thebe::Reference<Thebe::FramerateText> framerateText;
};