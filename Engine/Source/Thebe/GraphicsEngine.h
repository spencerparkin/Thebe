#pragma once

#include "Common.h"
#include <Windows.h>

namespace Thebe
{
	/**
	 * An instance of this class facilitates the rendering of graphics into a given window.
	 */
	class THEBE_API GraphicsEngine
	{
	public:
		GraphicsEngine();
		virtual ~GraphicsEngine();

		virtual bool Setup(HWND windowHandle);
		virtual void Shutdown();
		virtual void Render();

	protected:
		HWND windowHandle;
	};
}