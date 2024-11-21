#pragma once

#include "Thebe/Application.h"
#include "Thebe/GraphicsEngine.h"

class TestApplication : public Thebe::Application
{
public:
	TestApplication();
	virtual ~TestApplication();

	virtual bool PrepareForWindowShow() override;
	virtual void Shutdown(HINSTANCE instance) override;

private:
	Thebe::GraphicsEngine graphicsEngine;
};