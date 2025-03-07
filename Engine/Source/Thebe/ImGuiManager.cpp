#include "Thebe/ImGuiManager.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/CommandQueue.h"
#include "Thebe/EngineParts/RenderTarget.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/Profiler.h"
#include "Thebe/Log.h"

using namespace Thebe;

/*static*/ ImGuiManager* ImGuiManager::Get()
{
	static ImGuiManager manager;
	return &manager;
}

ImGuiManager::ImGuiManager()
{
	this->imGuiContext = nullptr;
	this->imPlotContext = nullptr;
	this->nextCookie = 1;
}

/*virtual*/ ImGuiManager::~ImGuiManager()
{
}

bool ImGuiManager::Setup(HWND windowHandle, GraphicsEngine* graphicsEngine)
{
	IMGUI_CHECKVERSION();

	this->imGuiContext = ImGui::CreateContext();
	if (!this->imGuiContext)
	{
		THEBE_LOG("Failed to create ImGui context!");
		return false;
	}

	this->imPlotContext = ImPlot::CreateContext();
	if (!this->imPlotContext)
	{
		THEBE_LOG("Failed to create ImPlot context!");
		return false;
	}

	if (!ImGui_ImplWin32_Init(windowHandle))
	{
		THEBE_LOG("Failed to initialize Win32 backend for ImGui.");
		return false;
	}

	this->descriptorPool.Set(new DescriptorPool());
	this->descriptorPool->SetGraphicsEngine(graphicsEngine);
	D3D12_DESCRIPTOR_HEAP_DESC& srvDescriptorPoolDesc = this->descriptorPool->GetDescriptorHeapDesc();
	srvDescriptorPoolDesc.NumDescriptors = 5 * 1024;
	srvDescriptorPoolDesc.NodeMask = 0;
	srvDescriptorPoolDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDescriptorPoolDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (!this->descriptorPool->Setup())
	{
		THEBE_LOG("Failed to SRV setup descriptor pool.");
		return false;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	SwapChain* swapChain = graphicsEngine->GetSwapChain();

	ImGui_ImplDX12_InitInfo initInfo{};
	initInfo.Device = graphicsEngine->GetDevice();
	initInfo.CommandQueue = graphicsEngine->GetCommandQueue()->GetCommandQueue();
	initInfo.NumFramesInFlight = swapChain->GetNumFrames();
	initInfo.RTVFormat = swapChain->GetRenderTargetFormat();
	initInfo.DSVFormat = swapChain->GetDepthStencileViewFormat();
	initInfo.UserData = this;
	initInfo.SrvDescriptorHeap = this->descriptorPool->GetDescriptorHeap();
	initInfo.SrvDescriptorAllocFn = &ImGuiManager::AllocSrvDescriptorEntryFunc;
	initInfo.SrvDescriptorFreeFn = &ImGuiManager::FreeSrvDescriptorEntryFunc;

	if (!ImGui_ImplDX12_Init(&initInfo))
	{
		THEBE_LOG("Failed to initialize DX12 backend for ImGui.");
		return false;
	}

	return true;
}

void ImGuiManager::Shutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();

	if (this->imPlotContext)
	{
		ImPlot::DestroyContext(this->imPlotContext);
		this->imPlotContext = nullptr;
	}

	if (this->imGuiContext)
	{
		ImGui::DestroyContext(this->imGuiContext);
		this->imGuiContext = nullptr;
	}

	if (this->descriptorPool)
	{
		this->descriptorPool->Shutdown();
		this->descriptorPool = nullptr;
	}
}

bool ImGuiManager::RegisterGuiCallback(ImGuiCallback callback, int& cookie)
{
	cookie = this->nextCookie++;
	CallbackEntry entry;
	entry.callback = callback;
	entry.enabled = false;
	this->callbackMap.insert(std::pair(cookie, entry));
	return true;
}

bool ImGuiManager::UnregisterGuiCallback(int& cookie)
{
	auto pair = this->callbackMap.find(cookie);
	if (pair == this->callbackMap.end())
		return false;

	this->callbackMap.erase(pair);
	cookie = 0;
	return true;
}

bool ImGuiManager::EnableGuiCallback(int cookie, bool enabled)
{
	auto pair = this->callbackMap.find(cookie);
	if (pair == this->callbackMap.end())
		return false;

	pair->second.enabled = enabled;
	return true;
}

bool ImGuiManager::IsGuiCallbackEnabled(int cookie)
{
	auto pair = this->callbackMap.find(cookie);
	if (pair == this->callbackMap.end())
		return false;

	return pair->second.enabled;
}

void ImGuiManager::BeginRender()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	for (auto& pair : this->callbackMap)
	{
		CallbackEntry& entry = pair.second;
		if (entry.enabled)
			entry.callback();
	}
}

void ImGuiManager::EndRender(ID3D12GraphicsCommandList* commandList)
{
	ImGui::Render();

	ID3D12DescriptorHeap* srvDescriptorHeap = this->descriptorPool->GetDescriptorHeap();
	commandList->SetDescriptorHeaps(1, &srvDescriptorHeap);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

/*static*/ LRESULT ImGuiManager::HandleWindowsMessage(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam);
	return ImGui_ImplWin32_WndProcHandler(windowHandle, msg, wParam, lParam);
}

/*static*/ void ImGuiManager::AllocSrvDescriptorEntryFunc(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
	auto manager = (ImGuiManager*)info->UserData;
	
	DescriptorPool::Descriptor descriptor;
	manager->AllocSrvDescriptor(descriptor);

	*out_cpu_desc_handle = descriptor.cpuHandle;
	*out_gpu_desc_handle = descriptor.gpuHandle;
}

/*static*/ void ImGuiManager::FreeSrvDescriptorEntryFunc(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
{
	auto manager = (ImGuiManager*)info->UserData;

	DescriptorPool::Descriptor descriptor;
	descriptor.cpuHandle = cpu_desc_handle;
	descriptor.gpuHandle = gpu_desc_handle;

	manager->FreeSrvDescriptor(descriptor);
}

void ImGuiManager::AllocSrvDescriptor(DescriptorPool::Descriptor& descriptor)
{
	bool succeeded = this->descriptorPool->AllocDescriptor(descriptor);
	THEBE_ASSERT_FATAL(succeeded);
}

void ImGuiManager::FreeSrvDescriptor(const DescriptorPool::Descriptor& descriptor)
{
	bool succeeded = this->descriptorPool->FreeDescriptor(descriptor);
	THEBE_ASSERT_FATAL(succeeded);
}