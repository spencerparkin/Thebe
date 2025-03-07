#pragma once

#include "Thebe/Common.h"
#include "Thebe/EngineParts/DescriptorPool.h"
#include "ImGui/imgui.h"
#include "ImPlot/implot.h"
#include "ImGui/backends/imgui_impl_win32.h"
#include "ImGui/backends/imgui_impl_dx12.h"
#include <functional>

namespace Thebe
{
	class GraphicsEngine;

	/**
	 * Provide support for using ImGui.  This is not necessarily an abstraction layer;
	 * just a convenience layer.  The engine and application are expected to make direct
	 * calls into the ImGui API.  The goal here is to have a home for common tasks.
	 */
	class THEBE_API ImGuiManager
	{
	public:
		ImGuiManager(GraphicsEngine* graphicsEngine);
		virtual ~ImGuiManager();

		bool Setup(HWND windowHandle);
		void Shutdown();
		void BeginRender();
		void EndRender(ID3D12GraphicsCommandList* commandList);
		static LRESULT HandleWindowsMessage(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam);

		typedef std::function<void()> ImGuiCallback;

		bool RegisterGuiCallback(ImGuiCallback callback, int& cookie);
		bool UnregisterGuiCallback(int& cookie);

	private:

		static void AllocSrvDescriptorEntryFunc(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
		static void FreeSrvDescriptorEntryFunc(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle);

		void AllocSrvDescriptor(DescriptorPool::Descriptor& descriptor);
		void FreeSrvDescriptor(const DescriptorPool::Descriptor& descriptor);

		GraphicsEngine* graphicsEngine;
		ImGuiContext* imGuiContext;
		ImPlotContext* imPlotContext;
		Reference<DescriptorPool> descriptorPool;
		int nextCookie;
		std::map<int, ImGuiCallback> callbackMap;
	};
}