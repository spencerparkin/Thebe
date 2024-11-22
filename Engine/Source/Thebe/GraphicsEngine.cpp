#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/MainRenderPass.h"
#include "Log.h"
#include <locale>
#include <codecvt>

using namespace Thebe;

GraphicsEngine::GraphicsEngine()
{
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

	// Get a factory we can use to enumerate adapters and create a swap-chain.
	ComPtr<IDXGIFactory4> factory;
	result = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
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
		result = factory->EnumAdapters1(i, &adapter);
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

	// TODO: I'd eventually like to be able to add a shadow pass, but that's a long way off.

	Reference<MainRenderPass> mainRenderPass = new MainRenderPass();
	mainRenderPass->SetGraphicsEngine(this);
	if (!mainRenderPass->Setup(windowHandle))
	{
		THEBE_LOG("Failed to setup main render pass.");
		return false;
	}

	this->renderPassArray.push_back(mainRenderPass.Get());

	return true;
}

void GraphicsEngine::Shutdown()
{
	for (Reference<RenderPass>& renderPass : this->renderPassArray)
		renderPass->Shutdown();

	this->renderPassArray.clear();
	this->device = nullptr;
}

void GraphicsEngine::Render()
{
	for (Reference<RenderPass>& renderPass : this->renderPassArray)
		renderPass->Perform();
}

ID3D12Device* GraphicsEngine::GetDevice()
{
	return this->device.Get();
}