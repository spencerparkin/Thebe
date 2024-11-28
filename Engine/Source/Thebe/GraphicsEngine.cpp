#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/MainRenderPass.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Texture.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/Shader.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/CommandExecutor.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/EngineParts/UploadHeap.h"
#include "Log.h"
#include "JsonValue.h"
#include <locale>
#include <ctype.h>
#include <codecvt>

using namespace Thebe;

GraphicsEngine::GraphicsEngine()
{
	this->frameCount = 0L;
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

	this->commandExecutor.Set(new CommandExecutor());
	this->commandExecutor->SetGraphicsEngine(this);
	if (!this->commandExecutor->Setup())
	{
		THEBE_LOG("Failed to setup command executor.");
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

	this->cbvDescriptorHeap.Set(new DescriptorHeap());
	this->cbvDescriptorHeap->SetGraphicsEngine(this);
	D3D12_DESCRIPTOR_HEAP_DESC& cbvDescriptorHeapDesc = this->cbvDescriptorHeap->GetDescriptorHeapDesc();
	cbvDescriptorHeapDesc.NumDescriptors = 512;
	cbvDescriptorHeapDesc.NodeMask = 0;
	cbvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (!this->cbvDescriptorHeap->Setup())
	{
		THEBE_LOG("Failed to setup descriptor heap for constants buffer views.");
		return false;
	}

	// TODO: I'd eventually like to be able to add a shadow pass, but that's a long way off.

	Reference<MainRenderPass> mainRenderPass = new MainRenderPass();
	mainRenderPass->SetGraphicsEngine(this);
	mainRenderPass->SetWindowHandle(windowHandle);
	if (!mainRenderPass->Setup())
	{
		THEBE_LOG("Failed to setup main render pass.");
		return false;
	}

	this->renderPassArray.push_back(mainRenderPass.Get());

#if defined THEBE_LOG_FRAMERATE
	this->clock.Reset();
#endif //THEBE_LOG_FRAMERATE

	return true;
}

void GraphicsEngine::Shutdown()
{
	this->WaitForGPUIdle();

	for (Reference<RenderPass>& renderPass : this->renderPassArray)
		renderPass->Shutdown();

	for (auto pair : this->enginePartCacheMap)
		pair.second->Shutdown();

	if (this->commandExecutor)
	{
		this->commandExecutor->Shutdown();
		this->commandExecutor = nullptr;
	}

	if (this->cbvDescriptorHeap)
	{
		this->cbvDescriptorHeap->Shutdown();
		this->cbvDescriptorHeap = nullptr;
	}

	this->pipelineStateCacheMap.clear();
	this->renderPassArray.clear();
	this->enginePartCacheMap.clear();

	// Do this almost last, because some shutdown routines may want to deallocate from the heap.
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

void GraphicsEngine::Render()
{
	for (Reference<RenderPass>& renderPass : this->renderPassArray)
		renderPass->Perform();

	this->frameCount++;

#if defined THEBE_LOG_FRAMERATE
	double deltaTimeSeconds = this->clock.GetCurrentTimeSeconds(true);
	this->frameTimeList.push_back(deltaTimeSeconds);
	while (this->frameTimeList.size() > THEBE_MAX_FRAMES_PER_FRAMERATE_CALCULATION)
		this->frameTimeList.pop_front();
	if ((this->frameCount % THEBE_FRAMES_PER_FRAMERATE_LOGGING) == 0)
		THEBE_LOG("Frame rate: %2.2f FPS", this->CalcFramerate());
#endif //THEBE_LOG_FRAMERATE
}

#if defined THEBE_LOG_FRAMERATE

double GraphicsEngine::CalcFramerate()
{
	if (this->frameTimeList.size() == 0)
		return 0.0;

	double averageFrameTimeSeconds = 0.0;
	for (double frameTimeSeconds : this->frameTimeList)
		averageFrameTimeSeconds += frameTimeSeconds;

	averageFrameTimeSeconds /= double(this->frameTimeList.size());
	return 1.0 / averageFrameTimeSeconds;
}

#endif //THEBE_LOG_FRAMERATE

void GraphicsEngine::WaitForGPUIdle()
{
	for (Reference<RenderPass>& renderPass : this->renderPassArray)
		renderPass->WaitForCommandQueueComplete();

	if (this->commandExecutor.Get())
		this->commandExecutor->WaitForCommandQueueComplete();
}

void GraphicsEngine::Resize(int width, int height)
{
	SwapChain* swapChain = this->GetSwapChain();
	if (swapChain)
		swapChain->Resize(width, height);
}

SwapChain* GraphicsEngine::GetSwapChain()
{
	auto mainRenderPass = this->FindRenderPass<MainRenderPass>();
	if (mainRenderPass)
		return dynamic_cast<SwapChain*>(mainRenderPass->GetOutput());

	return nullptr;
}

ID3D12Device* GraphicsEngine::GetDevice()
{
	return this->device.Get();
}

IDXGIFactory4* GraphicsEngine::GetFactory()
{
	return this->factory.Get();
}

CommandExecutor* GraphicsEngine::GetCommandExecutor()
{
	return this->commandExecutor;
}

DescriptorHeap* GraphicsEngine::GetCbvDescriptorHeap()
{
	return this->cbvDescriptorHeap;
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
			assetPath = this->assetFolder / assetPath;
		}
	}

	return std::filesystem::exists(assetPath);
}

bool GraphicsEngine::SetAssetFolder(std::filesystem::path assetFolder)
{
	if (assetFolder.is_relative() && !this->ResolvePath(assetFolder, RELATIVE_TO_EXECUTABLE))
	{
		THEBE_LOG("Could not locate folder: %s", assetFolder.string().c_str());
		return false;
	}

	this->assetFolder = assetFolder;
	return true;
}

const std::filesystem::path& GraphicsEngine::GetAssetFolder() const
{
	return this->assetFolder;
}

bool GraphicsEngine::GetRelativeToAssetFolder(std::filesystem::path& assetPath)
{
	if (!assetPath.is_absolute())
	{
		THEBE_LOG("Expected given path to be absolute.");
		return false;
	}

	assetPath = std::filesystem::relative(assetPath, this->assetFolder);
	return true;
}

ID3D12PipelineState* GraphicsEngine::GetOrCreatePipelineState(Material* material, VertexBuffer* vertexBuffer)
{
	std::string key = this->MakePipelineStateKey(material, vertexBuffer);
	auto iter = this->pipelineStateCacheMap.find(key);
	if (iter != this->pipelineStateCacheMap.end())
		return iter->second.Get();

	THEBE_LOG("Creating new PSO: %s.", key.c_str());

	const std::vector<D3D12_INPUT_ELEMENT_DESC>& elementDescriptionArray = vertexBuffer->GetElementDescArray();

	Shader* shader = material->GetShader();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout.NumElements = (UINT)elementDescriptionArray.size();
	psoDesc.InputLayout.pInputElementDescs = elementDescriptionArray.data();
	psoDesc.pRootSignature = shader->GetRootSignature();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader->GetVertexShaderBlob());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader->GetPixelShaderBlob());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.BlendState = material->GetBlendDesc();
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	ComPtr<ID3D12PipelineState> pipelineState;
	HRESULT result = this->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create graphics pipeline state object.  Error: 0x%08x", result);
		return nullptr;
	}

	this->pipelineStateCacheMap.insert(std::pair(key, pipelineState));
	return pipelineState.Get();
}

std::string GraphicsEngine::MakeAssetKey(const std::filesystem::path& assetPath)
{
	std::string key = assetPath.lexically_normal().string();
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) { return std::tolower(ch); });
	return key;
}

std::string GraphicsEngine::MakePipelineStateKey(const Material* material, const VertexBuffer* vertexBuffer)
{
	std::string key = std::format("{}_{}", material->GetHandle(), vertexBuffer->GetHandle());
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
	else if (ext == ".texture")
		enginePart.Set(new Texture());
	else if (ext == ".vertex_buffer")
		enginePart.Set(new VertexBuffer());
	else if (ext == ".index_buffer")
		enginePart.Set(new IndexBuffer());
	else if (ext == ".mesh")
		enginePart.Set(new Mesh());
	else if (ext == ".shader")
		enginePart.Set(new Shader());

	if (!enginePart.Get())
	{
		THEBE_LOG("Extension \"%s\" not recognized.", ext.c_str());
		return false;
	}

	enginePart->SetGraphicsEngine(this);

	std::filesystem::path relativePath = enginePartPath;
	if (!this->GetRelativeToAssetFolder(relativePath))
	{
		THEBE_LOG("Failed to get path (%s) relative to asset folder (%s).", enginePartPath.c_str(), this->assetFolder.string().c_str());
		return false;
	}

	if (!enginePart->LoadConfigurationFromJson(jsonRootValue.get(), relativePath))
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

	std::filesystem::path relativePath = enginePartPath;
	if (!this->GetRelativeToAssetFolder(relativePath))
	{
		THEBE_LOG("Failed to get path (%s) relative to asset folder (%s).", enginePartPath.c_str(), this->assetFolder.string().c_str());
		return false;
	}

	std::unique_ptr<JsonValue> jsonValue;
	if (!enginePart->DumpConfigurationToJson(jsonValue, relativePath) || !jsonValue.get())
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