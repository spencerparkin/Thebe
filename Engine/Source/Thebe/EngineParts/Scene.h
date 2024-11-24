#pragma once

#include "Thebe/EngineParts/RenderObject.h"

namespace Thebe
{
	class Space;

	/**
	 * 
	 */
	class THEBE_API Scene : public RenderObject
	{
	public:
		Scene();
		virtual ~Scene();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;

	protected:
		Reference<Space> rootSpace;
	};
}