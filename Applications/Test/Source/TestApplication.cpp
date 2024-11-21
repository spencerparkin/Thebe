#include "TestApplication.h"

TestApplication::TestApplication()
{
}

/*virtual*/ TestApplication::~TestApplication()
{
}

/*virtual*/ bool TestApplication::PrepareForWindowShow()
{
	if (!this->graphicsEngine.Setup(this->windowHandle))
		return false;

	return true;
}

/*virtual*/ void TestApplication::Shutdown(HINSTANCE instance)
{
	this->graphicsEngine.Shutdown();

	Application::Shutdown(instance);
}