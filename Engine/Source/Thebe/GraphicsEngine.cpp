#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/ShadowBuffer.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/CubeMapBuffer.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/CommandQueue.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/EngineParts/UploadHeap.h"
#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/Scene.h"
#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/Font.h"
#include "Thebe/Log.h"
#include "JsonValue.h"
#include <locale>
#include <ctype.h>
#include <codecvt>

namespace std
{
	template<>
	struct hash<D3D12_GRAPHICS_PIPELINE_STATE_DESC>
	{
		// This hash function is probably terrible.
		size_t operator()(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) const
		{
			size_t hashValue = 0;
			auto buffer = reinterpret_cast<const uint8_t*>(&psoDesc);
			uint32_t word = 0;
			for (uint32_t i = 0; i < sizeof(psoDesc); i++)
			{
				word |= uint32_t(buffer[i]) << ((i % 4) * 8);
				if (i % 4 == 3)
				{
					hashValue += std::hash<uint32_t>()(word);
					word = 0;
				}
			}
			hashValue += std::hash<uint32_t>()(word);
			return hashValue;
		}
	};
}

using namespace Thebe;

GraphicsEngine::GraphicsEngine()
{
	this->frameCount = 0L;
	this->deltaTimeSeconds = 0.0;
}

/*virtual*/ GraphicsEngine::~GraphicsEngine()
{
}

bool GraphicsEngine::Setup(HWND windowHandle)
{
	if (this->device.Get())
	{
		THEBE_LOG("Graphics engine already setup.");
		return false;
	}

	UINT dxgiFactoryFlags = 0;
	HRESULT result = 0;

	// Enable debug message from the underlying DX12 API if in debug mode.
#if defined _DEBUG
	ComPtr<ID3D12Debug> debugInterface;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	if (SUCCEEDED(result))
	{
		debugInterface->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	else
	{
		THEBE_LOG("Failed to get debug interface.  Error: 0x%08x", result);
		return false;
	}
#endif

	// Get a factory we can use to enumerate adapters.
	result = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&this->factory));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create DXGI factory.  Error: 0x%08x", result);
		return false;
	}

	// Look for a GPU that we can use.
	for (int i = 0; true; i++)
	{
		// Grab the next adapter.
		ComPtr<IDXGIAdapter1> adapter;
		result = this->factory->EnumAdapters1(i, &adapter);
		if (FAILED(result))
			break;

		// Skip any software-based adapters.  We want a physical GPU.
		DXGI_ADAPTER_DESC1 adapterDesc{};
		adapter->GetDesc1(&adapterDesc);
		if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			continue;

		// Does this adapter support DirectX 12?
		result = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(result))
		{
			// Yes.  Use it.
			std::string gpuDesc = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(adapterDesc.Description);
			THEBE_LOG("Using GPU: %s", gpuDesc.c_str());
			result = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), &this->device);
			if (FAILED(result))
			{
				THEBE_LOG("Failed to create D3D12 device.  Error: 0x%08x", result);
				return false;
			}

			break;
		}
	}

	if (!this->device.Get())
	{
		THEBE_LOG("Failed to find a D3D12-compatible device.");
		return false;
	}

	this->commandQueue.Set(new CommandQueue());
	this->commandQueue->SetGraphicsEngine(this);
	if (!this->commandQueue->Setup())
	{
		THEBE_LOG("Failed to setup command queue.");
		return false;
	}

	this->commandAllocator.Set(new CommandAllocator());
	this->commandAllocator->SetGraphicsEngine(this);
	if (!this->commandAllocator->Setup())
	{
		THEBE_LOG("Failed to setup command allocator.");
		return false;
	}

	this->uploadHeap.Set(new UploadHeap());
	this->uploadHeap->SetGraphicsEngine(this);
	this->uploadHeap->SetUploadBufferSize(THEBE_UPLOAD_HEAP_DEFAULT_SIZE);
	if (!this->uploadHeap->Setup())
	{
		THEBE_LOG("Failed to setup the upload heap.");
		return false;
	}

	this->csuDescriptorHeap.Set(new DescriptorHeap());
	this->csuDescriptorHeap->SetGraphicsEngine(this);
	D3D12_DESCRIPTOR_HEAP_DESC& csuDescriptorHeapDesc = this->csuDescriptorHeap->GetDescriptorHeapDesc();
	csuDescriptorHeapDesc.NumDescriptors = 512;
	csuDescriptorHeapDesc.NodeMask = 0;
	csuDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	csuDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (!this->csuDescriptorHeap->Setup())
	{
		THEBE_LOG("Failed to CSU setup descriptor heap.");
		return false;
	}

	this->rtvDescriptorHeap.Set(new DescriptorHeap());
	this->rtvDescriptorHeap->SetGraphicsEngine(this);
	D3D12_DESCRIPTOR_HEAP_DESC& rtvDescriptorHeapDesc = this->rtvDescriptorHeap->GetDescriptorHeapDesc();
	rtvDescriptorHeapDesc.NumDescriptors = 32;
	rtvDescriptorHeapDesc.NodeMask = 0;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (!this->rtvDescriptorHeap->Setup())
	{
		THEBE_LOG("Failed to setup RTV descriptor heap.");
		return false;
	}

	this->dsvDescriptorHeap.Set(new DescriptorHeap());
	this->dsvDescriptorHeap->SetGraphicsEngine(this);
	D3D12_DESCRIPTOR_HEAP_DESC& dsvDescriptorHeapDesc = this->dsvDescriptorHeap->GetDescriptorHeapDesc();
	dsvDescriptorHeapDesc.NumDescriptors = 32;
	dsvDescriptorHeapDesc.NodeMask = 0;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (!this->dsvDescriptorHeap->Setup())
	{
		THEBE_LOG("Failed to create DSV descriptor heap.");
		return false;
	}

	Reference<ShadowBuffer> shadowBuffer = new ShadowBuffer();
	shadowBuffer->SetGraphicsEngine(this);
	if (!shadowBuffer->Setup())
	{
		THEBE_LOG("Failed to setup shadow buffer.");
		return false;
	}

	Reference<SwapChain> swapChain = new SwapChain();
	swapChain->SetGraphicsEngine(this);
	swapChain->SetWindowHandle(windowHandle);
	if (!swapChain->Setup())
	{
		THEBE_LOG("Failed to setup swap-chain.");
		return false;
	}

	this->renderTargetArray.push_back(shadowBuffer.Get());
	this->renderTargetArray.push_back(swapChain.Get());
	
	this->clock.Reset();

	return true;
}

void GraphicsEngine::PurgeCache()
{
	for (auto pair : this->enginePartCacheMap)
		pair.second->Shutdown();

	this->enginePartCacheMap.clear();
}

void GraphicsEngine::RemoveExpiredPSOs(bool removeAllNow /*= false*/)
{
	std::vector<uint64_t> keyArray;

	for (auto pair : this->pipelineStateCacheMap)
	{
		PSO* pso = pair.second;
		if (pso->expirationCount == 0 || removeAllNow)
			keyArray.push_back(pair.first);
		else
			pso->expirationCount--;
	}

	for (uint64_t key : keyArray)
	{
		THEBE_LOG("Releasing PSO: %ull", key);
		auto pair = this->pipelineStateCacheMap.find(key);
		PSO* pso = pair->second;
		pso->pipelineState = nullptr;
		delete pso;
		this->pipelineStateCacheMap.erase(key);
	}
}

void GraphicsEngine::Shutdown()
{
	this->WaitForGPUIdle();

	this->PurgeCache();

	for (Reference<RenderTarget>& renderTarget : this->renderTargetArray)
		renderTarget->Shutdown();

	this->renderTargetArray.clear();

	this->RemoveExpiredPSOs(true);

	this->collisionSystem.UntrackAllObjects();

	if (this->commandQueue)
	{
		this->commandQueue->Shutdown();
		this->commandQueue = nullptr;
	}

	if (this->commandAllocator)
	{
		this->commandAllocator->Shutdown();
		this->commandAllocator = nullptr;
	}

	if (this->renderObject)
	{
		this->renderObject->Shutdown();
		this->renderObject = nullptr;
	}

	if (this->camera)
	{
		this->camera->Shutdown();
		this->camera = nullptr;
	}

	if (this->light)
	{
		this->light->Shutdown();
		this->light = nullptr;
	}

	// Shutdown heaps last of all, because the shutdown of other things may
	// want to deallocate out of these heaps.

	if (this->csuDescriptorHeap)
	{
		this->csuDescriptorHeap->Shutdown();
		this->csuDescriptorHeap = nullptr;
	}

	if (this->rtvDescriptorHeap)
	{
		this->rtvDescriptorHeap->Shutdown();
		this->rtvDescriptorHeap = nullptr;
	}

	if (this->dsvDescriptorHeap)
	{
		this->dsvDescriptorHeap->Shutdown();
		this->dsvDescriptorHeap = nullptr;
	}

	if (this->uploadHeap)
	{
		this->uploadHeap->Shutdown();
		this->uploadHeap = nullptr;
	}

	this->device = nullptr;
}

UINT64 GraphicsEngine::GetFrameCount()
{
	return this->frameCount;
}

UINT GraphicsEngine::GetFrameIndex()
{
	return UINT(this->frameCount % UINT64(THEBE_NUM_SWAP_FRAMES));
}

double GraphicsEngine::GetDeltaTime()
{
	return this->deltaTimeSeconds;
}

void GraphicsEngine::SetRenderObject(RenderObject* renderObject)
{
	this->renderObject = renderObject;
}

void GraphicsEngine::SetCamera(Camera* camera)
{
	this->camera = camera;
}

void GraphicsEngine::SetLight(Light* light)
{
	this->light = light;
}

void GraphicsEngine::SetEnvMap(CubeMapBuffer* envMap)
{
	this->envMap = envMap;
}

RenderObject* GraphicsEngine::GetRenderObject()
{
	return this->renderObject;
}

Camera* GraphicsEngine::GetCamera()
{
	return this->camera;
}

Light* GraphicsEngine::GetLight()
{
	return this->light;
}

CubeMapBuffer* GraphicsEngine::GetEnvMap()
{
	return this->envMap;
}

void GraphicsEngine::Render()
{
	if (!this->renderObject)
		return;

	renderObject->PrepareForRender();

	for (Reference<RenderTarget>& renderTarget : this->renderTargetArray)
		renderTarget->Render();

	this->RemoveExpiredPSOs();

	this->frameCount++;
	this->deltaTimeSeconds = this->clock.GetCurrentTimeSeconds(true);
}

void GraphicsEngine::WaitForGPUIdle()
{
	if (this->commandQueue.Get())
		this->commandQueue->WaitForCommandQueueComplete();
}

void GraphicsEngine::Resize(int width, int height)
{
	SwapChain* swapChain = this->GetSwapChain();
	if (swapChain)
		swapChain->Resize(width, height);

	if (this->camera.Get())
	{
		double aspectRatio = (height != 0) ? (double(width) / double(height)) : 1.0;
		camera->UpdateProjection(aspectRatio);
	}
}

SwapChain* GraphicsEngine::GetSwapChain()
{
	return this->FindRenderTarget<SwapChain>();
}

ID3D12Device* GraphicsEngine::GetDevice()
{
	return this->device.Get();
}

IDXGIFactory4* GraphicsEngine::GetFactory()
{
	return this->factory.Get();
}

CommandQueue* GraphicsEngine::GetCommandQueue()
{
	return this->commandQueue;
}

CommandAllocator* GraphicsEngine::GetCommandAllocator()
{
	return this->commandAllocator;
}

DescriptorHeap* GraphicsEngine::GetCSUDescriptorHeap()
{
	return this->csuDescriptorHeap;
}

DescriptorHeap* GraphicsEngine::GetRTVDescriptorHeap()
{
	return this->rtvDescriptorHeap;
}

DescriptorHeap* GraphicsEngine::GetDSVDescriptorHeap()
{
	return this->dsvDescriptorHeap;
}

UploadHeap* GraphicsEngine::GetUploadHeap()
{
	return this->uploadHeap;
}

bool GraphicsEngine::ResolvePath(std::filesystem::path& assetPath, ResolveMethod resolveMethod)
{
	if (assetPath.is_relative())
	{
		if (resolveMethod == RELATIVE_TO_EXECUTABLE)
		{
			char fileName[MAX_PATH];
			::GetModuleFileNameA(NULL, fileName, sizeof(fileName));

			std::filesystem::path basePath(fileName);
			basePath = basePath.parent_path();

			std::filesystem::path resolvedPath;
			while (std::distance(basePath.begin(), basePath.end()) > 0)
			{
				resolvedPath = basePath / assetPath;
				if (std::filesystem::exists(resolvedPath))
				{
					assetPath = resolvedPath;
					break;
				}

				basePath = basePath.parent_path();
			}
		}
		else if (resolveMethod == RELATIVE_TO_ASSET_FOLDER)
		{
			for (const std::filesystem::path& assetFolder : this->assetFolderList)
			{
				assetPath = assetFolder / assetPath;
				if (std::filesystem::exists(assetPath))
					break;
			}
		}
	}

	return std::filesystem::exists(assetPath);
}

bool GraphicsEngine::AddAssetFolder(std::filesystem::path assetFolder)
{
	if (assetFolder.is_relative() && !this->ResolvePath(assetFolder, RELATIVE_TO_EXECUTABLE))
	{
		THEBE_LOG("Could not locate folder: %s", assetFolder.string().c_str());
		return false;
	}

	this->assetFolderList.push_back(assetFolder);
	return true;
}

void GraphicsEngine::RemoveAllAssetFolders()
{
	this->assetFolderList.clear();
}

const std::list<std::filesystem::path>& GraphicsEngine::GetAssetFolderList() const
{
	return this->assetFolderList;
}

CollisionSystem* GraphicsEngine::GetCollisionSystem()
{
	return &this->collisionSystem;
}

bool GraphicsEngine::GleanAssetsFolderFromPath(const std::filesystem::path& assetPath, std::filesystem::path& assetsFolder)
{
	if (!assetPath.is_absolute())
		return false;

	assetsFolder = assetPath;

	while (std::distance(assetsFolder.begin(), assetsFolder.end()) > 0)
	{
		if (assetsFolder.stem().string() == "Assets")
			return true;

		assetsFolder = assetsFolder.parent_path();
	}

	return false;
}

bool GraphicsEngine::GetRelativeToAssetFolder(std::filesystem::path& assetPath, std::filesystem::path* assetFolderUsed /*= nullptr*/)
{
	if (!assetPath.is_absolute())
	{
		THEBE_LOG("Expected given path (%s) to be absolute.", assetPath.string().c_str());
		return false;
	}

	for (auto assetFolderUsed : this->assetFolderList)
	{
		std::error_code error;
		assetPath = std::filesystem::relative(assetPath, assetFolderUsed, error);
		if (error.value() == 0)
			return true;
	}
	
	THEBE_LOG("Failed to get path (%s) relative to a configured asset folder.", assetPath.string().c_str());
	return false;
}

ID3D12PipelineState* GraphicsEngine::GetOrCreatePipelineState(Material* material, VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer, RenderTarget* renderTarget)
{
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& elementDescriptionArray = vertexBuffer->GetElementDescArray();

	Shader* shader = material->GetShader();

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = indexBuffer ? indexBuffer->GetPrimitiveTopology() : vertexBuffer->GetPrimitiveTopology();
	D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

	switch (primitiveTopology)
	{
	case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
		primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
		primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	}

	THEBE_ASSERT(primitiveTopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout.NumElements = (UINT)elementDescriptionArray.size();
	psoDesc.InputLayout.pInputElementDescs = elementDescriptionArray.data();
	psoDesc.pRootSignature = shader->GetRootSignature();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader->GetVertexShaderBlob());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader->GetPixelShaderBlob());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.BlendState = material->GetBlendDesc();
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = primitiveTopologyType;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	renderTarget->ConfigurePiplineStateDesc(psoDesc);

	PSO* pso = nullptr;
	uint64_t key = this->MakePipelineStateKey(psoDesc);
	auto pair = this->pipelineStateCacheMap.find(key);
	if (pair != this->pipelineStateCacheMap.end())
		pso = pair->second;
	else
	{
		pso = new PSO();
		pso->expirationCount = 0;
		this->pipelineStateCacheMap.insert(std::pair(key, pso));

		HRESULT result = this->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso->pipelineState));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create graphics pipeline state object.  Error: 0x%08x", result);
			return nullptr;
		}

		THEBE_LOG("Created PSO: %ull", key);
	}

	pso->expirationCount = THEBE_PSO_MAX_EXPIRATION_COUNT;
	return pso->pipelineState.Get();
}

std::string GraphicsEngine::MakeAssetKey(const std::filesystem::path& assetPath)
{
	std::string key = assetPath.lexically_normal().string();
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) { return std::tolower(ch); });
	return key;
}

uint64_t GraphicsEngine::MakePipelineStateKey(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc)
{
	std::hash<D3D12_GRAPHICS_PIPELINE_STATE_DESC> hash;
	uint64_t key = hash(pipelineStateDesc);
	return key;
}

bool GraphicsEngine::LoadEnginePartFromFile(std::filesystem::path enginePartPath, Reference<EnginePart>& enginePart, uint32_t flags /*= 0*/)
{
	using namespace ParseParty;

	if (!this->ResolvePath(enginePartPath, RELATIVE_TO_ASSET_FOLDER))
	{
		THEBE_LOG("Failed to resolved path: %s", enginePartPath.string().c_str());
		return false;
	}

	if ((flags & THEBE_LOAD_FLAG_DONT_CHECK_CACHE) == 0)
	{
		std::string key = this->MakeAssetKey(enginePartPath);
		auto iter = this->enginePartCacheMap.find(key);
		if (iter != this->enginePartCacheMap.end())
		{
			THEBE_LOG("Got asset %s from cache.", enginePartPath.string().c_str());
			enginePart.Set(iter->second);
			return true;
		}
	}

	std::ifstream fileStream;
	fileStream.open(enginePartPath.string(), std::ios::in);
	if (!fileStream.is_open())
	{
		THEBE_LOG("Failed to open (for reading) the file: %s", enginePartPath.string().c_str());
		return false;
	}

	std::stringstream stringStream;
	stringStream << fileStream.rdbuf();
	std::string jsonString = stringStream.str();
	std::string parseError;
	std::unique_ptr<JsonValue> jsonRootValue(JsonValue::ParseJson(jsonString, parseError));
	if (!jsonRootValue.get())
	{
		THEBE_LOG("Json parse error in file: %s", enginePartPath.string().c_str());
		THEBE_LOG("Json parse error: %s", parseError.c_str());
		return false;
	}

	std::string ext = enginePartPath.extension().string();
	enginePart.Reset();
	if (ext == ".material")
		enginePart.Set(new Material());
	else if (ext == ".texture_buffer")
		enginePart.Set(new TextureBuffer());
	else if (ext == ".vertex_buffer")
		enginePart.Set(new VertexBuffer());
	else if (ext == ".index_buffer")
		enginePart.Set(new IndexBuffer());
	else if (ext == ".mesh")
		enginePart.Set(new Mesh());
	else if (ext == ".shader")
		enginePart.Set(new Shader());
	else if (ext == ".scene")
		enginePart.Set(new Scene());
	else if (ext == ".cube_map")
		enginePart.Set(new CubeMapBuffer());
	else if (ext == ".font")
		enginePart.Set(new Font());

	if (!enginePart.Get())
	{
		THEBE_LOG("Extension \"%s\" not recognized.", ext.c_str());
		return false;
	}

	enginePart->SetGraphicsEngine(this);

	if (!enginePart->LoadConfigurationFromJson(jsonRootValue.get(), enginePartPath))
	{
		THEBE_LOG("Engine part failed to configure from JSON.");
		return false;
	}

	if (!enginePart->Setup())
	{
		THEBE_LOG("Failed to setup engine part.");
		return false;
	}

	if ((flags & THEBE_LOAD_FLAG_DONT_CACHE_PART) == 0)
	{
		std::string key = this->MakeAssetKey(enginePartPath);
		this->enginePartCacheMap.insert(std::pair(key, enginePart));
	}

	THEBE_LOG("Loaded asset: %s", enginePartPath.string().c_str());
	return true;
}

bool GraphicsEngine::DumpEnginePartToFile(std::filesystem::path enginePartPath, const EnginePart* enginePart, uint32_t flags /*= 0*/)
{
	using namespace ParseParty;

	std::unique_ptr<JsonValue> jsonValue;
	if (!enginePart->DumpConfigurationToJson(jsonValue, enginePartPath) || !jsonValue.get())
	{
		THEBE_LOG("Failed to dump engine part configuration to JSON.");
		return false;
	}

	if (std::filesystem::exists(enginePartPath))
	{
		if ((flags & THEBE_DUMP_FLAG_CAN_OVERWRITE) == 0)
		{
			THEBE_LOG("Cannot overwrite existing file: %s", enginePartPath.string().c_str());
			return false;
		}

		if (!std::filesystem::remove(enginePartPath))
		{
			THEBE_LOG("Failed to delete file: %s", enginePartPath.string().c_str());
			return false;
		}
	}

	// Note that we don't check the extension here, but maybe we should?
	std::ofstream fileStream;
	fileStream.open(enginePartPath.string(), std::ios::out);
	if (!fileStream.is_open())
	{
		THEBE_LOG("Failed to open (for writing) the file: %s", enginePartPath.string().c_str());
		return false;
	}

	std::string jsonString;
	if (!jsonValue->PrintJson(jsonString))
	{
		THEBE_LOG("Failed to print JSON to string.");
		return false;
	}

	fileStream << jsonString;
	fileStream.close();

	return true;
}