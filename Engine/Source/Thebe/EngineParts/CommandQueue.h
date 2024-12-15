#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/EngineParts/Fence.h"
#include <d3d12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * 
	 */
	class THEBE_API CommandQueue : public EnginePart
	{
	public:
		CommandQueue();
		virtual ~CommandQueue();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void WaitForCommandQueueComplete();

		bool ExecuteCommandList(ID3D12CommandList* commandList);

		ID3D12CommandQueue* GetCommandQueue();

	protected:
		Reference<Fence> fence;
		ComPtr<ID3D12CommandQueue> commandQueue;
	};
}