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
	virtual LRESULT OnPaint(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT OnSize(WPARAM wParam, LPARAM lParam) override;

private:
	Thebe::Reference<Thebe::GraphicsEngine> graphicsEngine;
};