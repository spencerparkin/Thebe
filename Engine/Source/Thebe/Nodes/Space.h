#pragma once

#include "RenderObject.h"
#include "Math/Transform.h"

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

	protected:
		Transform childToParent;
		std::vector<Reference<Space>> subSpaceArray;
	};
}