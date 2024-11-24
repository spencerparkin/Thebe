#pragma once

#include "Thebe/EnginePart.h"
#include <wrl.h>
#include <d3d12.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * These are used for CPU/GPU synchronization.
	 */
	class THEBE_API Fence : public EnginePart
	{
	public:
		Fence();
		virtual ~Fence();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void EnqueueSignalAndWaitForIt(ID3D12CommandQueue* commandQueue);
		void EnqueueSignal(ID3D12CommandQueue* commandQueue);
		void WaitForSignalIfNecessary();

	protected:
		HANDLE eventHandle;
		ComPtr<ID3D12Fence> fence;
		UINT64 count;
	};
}