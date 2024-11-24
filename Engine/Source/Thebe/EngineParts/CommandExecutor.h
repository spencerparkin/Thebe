#pragma once

#include "Thebe/EngineParts/CommandQueue.h"
#include <d3d12.h>
#include <wrl.h>

namespace Thebe
{
	using Microsoft::WRL::ComPtr;

	/**
	 * 
	 */
	class THEBE_API CommandExecutor : public CommandQueue
	{
	public:
		CommandExecutor();
		virtual ~CommandExecutor();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		bool BeginRecording(ComPtr<ID3D12GraphicsCommandList>& commandList);
		bool EndRecording(ComPtr<ID3D12GraphicsCommandList> commandList);
		void Execute();

	protected:
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		std::vector<ComPtr<ID3D12GraphicsCommandList>> commandListArray;
	};
}