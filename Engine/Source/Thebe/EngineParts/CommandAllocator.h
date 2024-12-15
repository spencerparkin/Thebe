#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/EngineParts/Fence.h"
#include <wrl.h>
#include <d3d12.h>
#include <functional>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * Note that commands in a command list are guarenteed to be executed in order (synchronously.)
	 * However, command lists submitted to a command queue have no such guarentee, because they
	 * are executed asynchronously by the GPU.  Synchronization on the GPU across command lists
	 * is accomplished using resource barriers.  Synchronization on the GPU across command queues
	 * would require intervention on the CPU.
	 */
	class THEBE_API CommandAllocator : public EnginePart
	{
	public:
		CommandAllocator();
		virtual ~CommandAllocator();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		/**
		 * This will wait on the GPU if necessary before returning.
		 */
		bool BeginRecordingCommandList();

		/**
		 * This will submit the recorded command-list to the main command queue.
		 */
		bool EndRecordingCommandList();

		ID3D12GraphicsCommandList* GetCommandList();

	protected:
		virtual void PreSignal();
	
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12GraphicsCommandList> commandList;
		Reference<Fence> fence;
	};
}