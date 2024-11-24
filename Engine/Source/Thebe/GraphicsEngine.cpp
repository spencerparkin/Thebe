#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/MainRenderPass.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/Material.h"
#include "Thebe/EngineParts/Texture.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/Mesh.h"
#include "Thebe/EngineParts/CommandExecutor.h"
#include "Log.h"
#include "JsonValue.h"
#include <locale>
#include <codecvt>

using namespace Thebe;

GraphicsEngine::GraphicsEngine()
{
#if defined THEBE_LOG_FRAMERATE
	this->frameCount = 0L;
#endif //THEBE_LOG_FRAMERATE
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

	this->renderPassArray.clear();
	this->commandExecutor = nullptr;
	this->device = nullptr;
}

void GraphicsEngine::Render()
{
	for (Reference<RenderPass>& renderPass : this->renderPassArray)
		renderPass->Perform();

#if defined THEBE_LOG_FRAMERATE
	double deltaTimeSeconds = this->clock.GetCurrentTimeSeconds(true);
	this->frameTimeList.push_back(deltaTimeSeconds);
	while (this->frameTimeList.size() > THEBE_MAX_FRAMES_PER_FRAMERATE_CALCULATION)
		this->frameTimeList.pop_front();
	if ((this->frameCount++ % THEBE_FRAMES_PER_FRAMERATE_LOGGING) == 0)
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
	return this->commandExecutor.Get();
}

bool GraphicsEngine::LoadEnginePartFromFile(const std::filesystem::path& enginePartPath, Reference<EnginePart>& enginePart, uint32_t flags /*= 0*/)
{
	using namespace ParseParty;

	if ((flags & THEBE_LOAD_FLAG_DONT_CHECK_CACHE) == 0)
	{
		// TODO: We need to first try to hit a cache.
	}

	if (!std::filesystem::exists(enginePartPath))
	{
		THEBE_LOG("Can't load part, because file (%s) does not exist.", enginePartPath.c_str());
		return false;
	}

	std::ifstream fileStream;
	fileStream.open(enginePartPath.string(), std::ios::in);
	if (!fileStream.is_open())
	{
		THEBE_LOG("Failed to open (for reading) the file: %s", enginePartPath.c_str());
		return false;
	}

	std::stringstream stringStream;
	stringStream << fileStream.rdbuf();
	std::string jsonString = stringStream.str();
	std::string parseError;
	std::unique_ptr<JsonValue> jsonRootValue(JsonValue::ParseJson(jsonString, parseError));
	if (!jsonRootValue.get())
	{
		THEBE_LOG("Json parse error in file: %s", enginePartPath.c_str());
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
	else if (ext == ".mesh")
		enginePart.Set(new Mesh());
	// TODO: Add Shader, PSO, RootSignature?

	if (!enginePart.Get())
	{
		THEBE_LOG("Extension \"%s\" not recognized.", ext.c_str());
		return false;
	}

	if (!enginePart->LoadConfigurationFromJson(jsonRootValue.get()))
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
		// TODO: Insert the part in a cache we can check later.
	}

	return true;
}

bool GraphicsEngine::DumpEnginePartToFile(const std::filesystem::path& enginePartPath, const EnginePart* enginePart, uint32_t flags /*= 0*/)
{
	using namespace ParseParty;

	std::unique_ptr<JsonValue> jsonValue;
	if (!enginePart->DumpConfigurationToJson(jsonValue) || !jsonValue.get())
	{
		THEBE_LOG("Failed to dump engine part configuration to JSON.");
		return false;
	}

	if (std::filesystem::exists(enginePartPath))
	{
		if ((flags & THEBE_DUMP_FLAG_CAN_OVERWRITE) == 0)
		{
			THEBE_LOG("Cannot overwrite existing file: %s", enginePartPath.c_str());
			return false;
		}

		if (!std::filesystem::remove(enginePartPath))
		{
			THEBE_LOG("Failed to delete file: %s", enginePartPath.c_str());
			return false;
		}
	}

	// Note that we don't check the extension here, but maybe we should?
	std::ofstream fileStream;
	fileStream.open(enginePartPath.string(), std::ios::out);
	if (!fileStream.is_open())
	{
		THEBE_LOG("Failed to open (for writing) the file: %s", enginePartPath.c_str());
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