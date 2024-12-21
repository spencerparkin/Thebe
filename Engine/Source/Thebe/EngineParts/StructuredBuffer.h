#pragma once

#include "Thebe/EngineParts/Buffer.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API StructuredBuffer : public Buffer
	{
	public:
		StructuredBuffer();
		virtual ~StructuredBuffer();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool CreateResourceView(CD3DX12_CPU_DESCRIPTOR_HANDLE& handle, ID3D12Device* device) override;

		void SetStructSize(UINT64 structSize);
		UINT64 GetStructSize() const;

		UINT64 GetNumStructs() const;

		template<typename T>
		T* GetStructure(UINT64 i)
		{
			THEBE_ASSERT_FATAL(sizeof(T) == this->structSize);
			THEBE_ASSERT_FATAL(i < this->GetNumStructs());
			return &reinterpret_cast<T*>(this->originalBuffer.data())[i];
		}

	protected:

		UINT64 structSize;
	};
}