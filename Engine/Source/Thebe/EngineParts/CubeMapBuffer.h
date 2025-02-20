#pragma once

#include "Thebe/EngineParts/Buffer.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API CubeMapBuffer : public Buffer
	{
	public:
		CubeMapBuffer();
		virtual ~CubeMapBuffer();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;
		virtual bool CreateResourceView(CD3DX12_CPU_DESCRIPTOR_HANDLE& handle, ID3D12Device* device) override;

	protected:
		virtual bool ValidateBufferDescription() override;
		virtual UINT64 GetUploadHeapAllocationSize(ID3D12Device* device) override;
		virtual bool CopyDataToUploadHeap(UINT8* uploadBuffer, ID3D12Device* device) override;
		virtual void CopyDataFromUploadHeapToDefaultHeap(UploadHeap* uploadHeap, ID3D12GraphicsCommandList* commandList, ID3D12Device* device) override;
	};
}