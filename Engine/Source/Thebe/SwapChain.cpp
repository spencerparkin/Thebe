#include "SwapChain.h"

using namespace Thebe;

SwapChain::SwapChain()
{
	this->windowHandle = NULL;
}

/*virtual*/ SwapChain::~SwapChain()
{
}

/*virtual*/ bool SwapChain::Setup(HWND windowHandle)
{
	if (this->windowHandle)
		return false;

	this->windowHandle = windowHandle;

	return true;
}