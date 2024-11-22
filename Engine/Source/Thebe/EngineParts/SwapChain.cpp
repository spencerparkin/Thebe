#include "Thebe/EngineParts/SwapChain.h"

using namespace Thebe;

SwapChain::SwapChain()
{
	this->windowHandle = NULL;
}

/*virtual*/ SwapChain::~SwapChain()
{
}

/*virtual*/ bool SwapChain::Setup(void* data)
{
	if (this->windowHandle)
		return false;

	this->windowHandle = windowHandle;

	//...

	return true;
}

/*virtual*/ void SwapChain::Shutdown()
{
	//...
}

void SwapChain::Resize(int width, int height)
{
	if (width == 0 || height == 0)
	{
		// TODO: Get width/height from cached window handle.
	}

	// TODO: Share code here with setup.
}