#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/Clock.h"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <wrl.h>
#include <dxgi1_6.h>

#define THEBE_FRAMES_PER_FRAMERATE_LOGGING				64
#define THEBE_MAX_FRAMES_PER_FRAMERATE_CALCULATION		32

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class RenderPass;

	/**
	 * An instance of this class facilitates the rendering of graphics into a window.
	 * An application can instantiate multiple instances of this class to render into
	 * multiple windows, if desired.  Typically, just one window is used.
	 */
	class THEBE_API GraphicsEngine : public ReferenceCounted
	{
	public:
		GraphicsEngine();
		virtual ~GraphicsEngine();

		/**
		 * Initialize the graphics engine.
		 * 
		 * @param[in] windowHandle A swap-chain is created for this window.
		 */
		bool Setup(HWND windowHandle);

		/**
		 * Shut every thing down, at which point, @ref Setup could be called again.
		 */
		void Shutdown();

		/**
		 * Kick-off all the render passes in sequence.  This is to be called once per
		 * iteration of the main program loop to render into the window.
		 */
		void Render();

		/**
		 * Upon return from this call, any work we've submitted to the GPU
		 * should be finished, and the GPU should be sitting around doing nothing.
		 */
		void WaitForGPUIdle();

		/**
		 * Call this when the window dimensions change.
		 */
		void Resize(int width, int height);

		ID3D12Device* GetDevice();
		IDXGIFactory4* GetFactory();

	private:

		template<typename T>
		T* FindRenderPass()
		{
			for (Reference<RenderPass>& renderPass : this->renderPassArray)
			{
				T* typedRenderPass = dynamic_cast<T*>(renderPass.Get());
				if (typedRenderPass)
					return typedRenderPass;
			}

			return nullptr;
		}

		ComPtr<ID3D12Device> device;
		ComPtr<IDXGIFactory4> factory;
		std::vector<Reference<RenderPass>> renderPassArray;

#if defined THEBE_LOG_FRAMERATE
		double CalcFramerate();
		Clock clock;
		std::list<double> frameTimeList;
		UINT64 frameCount;
#endif //THEBE_LOG_FRAMERATE
	};
}