#include "GraphicsEngine.h"
#include "Log.h"

using namespace Thebe;

GraphicsEngine::GraphicsEngine()
{
	this->windowHandle = NULL;
}

/*virtual*/ GraphicsEngine::~GraphicsEngine()
{
}

/*virtual*/ bool GraphicsEngine::Setup(HWND windowHandle)
{
	if (this->windowHandle != NULL)
		return false;

	this->windowHandle = windowHandle;

	//...

	return true;
}

/*virtual*/ void GraphicsEngine::Shutdown()
{
	//...

	this->windowHandle = NULL;
}

/*virtual*/ void GraphicsEngine::Render()
{
	//...
}