#pragma once

#include "Node.h"
#include "Transform.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API Camera : public Node
	{
	public:
		Camera();
		virtual ~Camera();

	private:
		Transform cameraToWorld;
	};
}