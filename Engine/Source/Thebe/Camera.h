#pragma once

#include "Reference.h"
#include "Transform.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API Camera : public ReferenceCounted
	{
	public:
		Camera();
		virtual ~Camera();

	private:
		Transform cameraToWorld;
	};
}