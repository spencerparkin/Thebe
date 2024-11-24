#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/Clock.h"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <filesystem>

#define THEBE_FRAMES_PER_FRAMERATE_LOGGING				64
#define THEBE_MAX_FRAMES_PER_FRAMERATE_CALCULATION		32
#define THEBE_LOAD_FLAG_DONT_CHECK_CACHE				0x00000001
#define THEBE_LOAD_FLAG_DONT_CACHE_PART					0x00000002
#define THEBE_DUMP_FLAG_CAN_OVERWRITE					0x00000001

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class RenderPass;
	class EnginePart;
	class SwapChain;
	class CommandExecutor;

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
		SwapChain* GetSwapChain();
		CommandExecutor* GetCommandExecutor();

		bool LoadEnginePartFromFile(const std::filesystem::path& enginePartPath, Reference<EnginePart>& enginePart, uint32_t flags = 0);
		bool DumpEnginePartToFile(const std::filesystem::path& enginePartPath, const EnginePart* enginePart, uint32_t flags = 0);

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
		Reference<CommandExecutor> commandExecutor;

#if defined THEBE_LOG_FRAMERATE
		double CalcFramerate();
		Clock clock;
		std::list<double> frameTimeList;
		UINT64 frameCount;
#endif //THEBE_LOG_FRAMERATE
	};
}