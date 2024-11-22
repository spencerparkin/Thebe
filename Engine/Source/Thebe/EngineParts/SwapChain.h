#pragma once

#include "Thebe/EngineParts/RenderTarget.h"
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

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

		void Resize(int width, int height);

		//virtual void PreRender(CommandList* commandList) override;		// Stall if we can't render the frame yet.
		//virtual void PostRender(CommandList* commandList);				// Present/flip and signal the current frame.

	private:
		HWND windowHandle;
		// TODO: Own an array of frames, each owning a command allocator and fence/event objects.
	};
}