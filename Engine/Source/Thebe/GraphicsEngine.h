#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/Utilities/Clock.h"
#include "Thebe/EngineParts/CubeMapBuffer.h"
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
#define THEBE_UPLOAD_HEAP_DEFAULT_SIZE					128 * 1024 * 1024
#define THEBE_PSO_MAX_EXPIRATION_COUNT					512

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	class CommandQueue;
	class CommandAllocator;
	class RenderTarget;
	class EnginePart;
	class SwapChain;
	class DescriptorHeap;
	class Material;
	class VertexBuffer;
	class UploadHeap;
	class RenderObject;
	class Camera;
	class Light;
	class CubeMapBuffer;

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
		CommandQueue* GetCommandQueue();
		CommandAllocator* GetCommandAllocator();
		DescriptorHeap* GetCSUDescriptorHeap();
		DescriptorHeap* GetRTVDescriptorHeap();
		DescriptorHeap* GetDSVDescriptorHeap();
		UploadHeap* GetUploadHeap();

		void SetRenderObject(RenderObject* renderObject);
		void SetCamera(Camera* camera);
		void SetLight(Light* light);
		void SetEnvMap(CubeMapBuffer* envMap);

		RenderObject* GetRenderObject();
		Camera* GetCamera();
		Light* GetLight();
		CubeMapBuffer* GetEnvMap();

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
		bool GleanAssetsFolderFromPath(const std::filesystem::path& assetPath, std::filesystem::path& assetsFolder);

		UINT64 GetFrameCount();
		UINT GetFrameIndex();
		double GetDeltaTime();

		ID3D12PipelineState* GetOrCreatePipelineState(Material* material, VertexBuffer* vertexBuffer, RenderTarget* renderTarget);

		template<typename T>
		T* FindRenderTarget()
		{
			for (Reference<RenderTarget>& renderTarget : this->renderTargetArray)
			{
				T* typedRenderTarget = dynamic_cast<T*>(renderTarget.Get());
				if (typedRenderTarget)
					return typedRenderTarget;
			}

			return nullptr;
		}

	private:
		struct PSO
		{
			ComPtr<ID3D12PipelineState> pipelineState;
			UINT expirationCount;
		};

		ComPtr<ID3D12Device> device;
		ComPtr<IDXGIFactory4> factory;
		std::vector<Reference<RenderTarget>> renderTargetArray;
		Reference<CommandQueue> commandQueue;
		Reference<CommandAllocator> commandAllocator;
		Reference<UploadHeap> uploadHeap;
		Reference<DescriptorHeap> csuDescriptorHeap;
		Reference<DescriptorHeap> rtvDescriptorHeap;
		Reference<DescriptorHeap> dsvDescriptorHeap;
		std::list<std::filesystem::path> assetFolderList;
		std::unordered_map<std::string, Reference<EnginePart>> enginePartCacheMap;
		std::unordered_map<uint64_t, PSO*> pipelineStateCacheMap;
		Reference<RenderObject> renderObject;
		Reference<Camera> camera;
		Reference<Light> light;
		Reference<CubeMapBuffer> envMap;

		std::string MakeAssetKey(const std::filesystem::path& assetPath);
		uint64_t MakePipelineStateKey(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc);
		void RemoveExpiredPSOs(bool removeAllNow = false);

		double CalcFramerate();
		Clock clock;
		std::list<double> frameTimeList;
		UINT64 frameCount;
		double deltaTimeSeconds;
	};
}