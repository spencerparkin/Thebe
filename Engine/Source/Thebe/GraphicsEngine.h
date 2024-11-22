#pragma once

#include "Common.h"
#include "Reference.h"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <wrl.h>
#include <dxgi1_6.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class RenderPass;
	class SwapChain;

	/**
	 * An instance of this class facilitates the rendering of graphics.
	 */
	class THEBE_API GraphicsEngine : public ReferenceCounted
	{
	public:
		GraphicsEngine();
		virtual ~GraphicsEngine();

		/**
		 * Initialize the graphics engine.
		 * 
		 * @param[in] windowHandle A swap-chain is created for this window, if given.
		 */
		virtual bool Setup(HWND windowHandle = NULL);

		/**
		 * Shut every thing down, at which point, @ref Setup could be called again.
		 */
		virtual void Shutdown();

		/**
		 * Kick-off all the render passes in sequence.  This is to be called once per
		 * iteration of the main program loop to render into a window or off-screen.
		 */
		virtual void Render();

		/**
		 * Add a pass to the rendering sequence.  Render passes are kicked-off in order.
		 */
		bool AddRenderPass(RenderPass* renderPass);

		ID3D12Device* GetDevice();
		SwapChain* GetSwapChain();

	private:
		ComPtr<ID3D12Device> device;
		Reference<SwapChain> swapChain;
		std::vector<Reference<RenderPass>> renderPassArray;
	};
}