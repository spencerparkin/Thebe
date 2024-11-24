#pragma once

#include "Thebe/EngineParts/RenderObject.h"
#include "Thebe/Math/Transform.h"

namespace Thebe
{
	/**
	 * These nodes make up the scene hierarchy.
	 */
	class THEBE_API Space : public RenderObject
	{
	public:
		Space();
		virtual ~Space();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool Render(ID3D12GraphicsCommandList* commandList, Camera* camera) override;

	protected:
		Transform childToParent;
		Transform objectToWorld;
		std::vector<Reference<Space>> subSpaceArray;
	};
}