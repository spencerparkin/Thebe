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

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

	protected:
		Transform childToParent;
		std::vector<Reference<Space>> subSpaceArray;
	};
}