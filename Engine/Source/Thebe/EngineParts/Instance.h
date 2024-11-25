#pragma once

#include "Thebe/EngineParts/Space.h"

namespace Thebe
{
	/**
	 *
	 */
	class THEBE_API Instance : public Space
	{
	public:
		Instance();
		virtual ~Instance();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;

	private:
		Reference<RenderObject> renderObject;
	};
}