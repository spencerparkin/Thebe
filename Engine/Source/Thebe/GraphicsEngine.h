#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/Utilities/Clock.h"
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
#define THEBE_UPLOAD_HEAP_DEFAULT_SIZE					64 * 1024 * 1024

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class RenderPass;
	class EnginePart;
	class SwapChain;
	class CommandExecutor;
	class DescriptorHeap;
	class Material;
	class VertexBuffer;
	class UploadHeap;
	class RenderObject;
	class Camera;

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
		DescriptorHeap* GetCSUDescriptorHeap();
		DescriptorHeap* GetRTVDescriptorHeap();
		DescriptorHeap* GetDSVDescriptorHeap();
		UploadHeap* GetUploadHeap();

		void SetInputToAllRenderPasses(RenderObject* renderObject);
		void SetCameraForMainRenderPass(Camera* camera);
		void SetCameraForShadowPass(Camera* camera);

		bool LoadEnginePartFromFile(std::filesystem::path enginePartPath, Reference<EnginePart>& enginePart, uint32_t flags = 0);
		bool DumpEnginePartToFile(std::filesystem::path enginePartPath, const EnginePart* enginePart, uint32_t flags = 0);

		template<typename T>
		bool LoadEnginePartFromFile(std::filesystem::path enginePartPath, Reference<T>& enginePartTyped, uint32_t flags = 0)
		{
			Reference<EnginePart> enginePart;
			if (!this->LoadEnginePartFromFile(enginePartPath, enginePart, flags))
				return false;

			enginePartTyped.SafeSet(enginePart.Get());
			return enginePartTyped.Get() ? true : true;
		}

		void PurgeCache();

		enum ResolveMethod
		{
			RELATIVE_TO_EXECUTABLE,
			RELATIVE_TO_ASSET_FOLDER
		};

		bool ResolvePath(std::filesystem::path& assetPath, ResolveMethod resolveMethod);
		bool AddAssetFolder(std::filesystem::path assetFolder);
		void RemoveAllAssetFolders();
		const std::list<std::filesystem::path>& GetAssetFolderList() const;
		bool GetRelativeToAssetFolder(std::filesystem::path& assetPath, std::filesystem::path* assetFolderUsed = nullptr);

		UINT64 GetFrameCount();
		double GetDeltaTime();

		ID3D12PipelineState* GetOrCreatePipelineState(Material* material, VertexBuffer* vertexBuffer);

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
		Reference<UploadHeap> uploadHeap;
		Reference<DescriptorHeap> csuDescriptorHeap;
		Reference<DescriptorHeap> rtvDescriptorHeap;
		Reference<DescriptorHeap> dsvDescriptorHeap;
		std::list<std::filesystem::path> assetFolderList;
		std::unordered_map<std::string, Reference<EnginePart>> enginePartCacheMap;
		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> pipelineStateCacheMap;

		std::string MakeAssetKey(const std::filesystem::path& assetPath);
		std::string MakePipelineStateKey(const Material* material, const VertexBuffer* vertexBuffer);

		double CalcFramerate();
		Clock clock;
		std::list<double> frameTimeList;
		UINT64 frameCount;
		double deltaTimeSeconds;
	};
}