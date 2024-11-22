#pragma once

#include "RenderTarget.h"
#include <Windows.h>

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API SwapChain : public RenderTarget
	{
	public:
		SwapChain();
		virtual ~SwapChain();

		virtual bool Setup(HWND windowHandle);

	private:
		HWND windowHandle;
		// TODO: Own an array of frames, each owning a command allocator and fence/event objects.
	};
}