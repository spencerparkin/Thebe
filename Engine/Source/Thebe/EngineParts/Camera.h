#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/Math/Transform.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API Camera : public EnginePart
	{
	public:
		Camera();
		virtual ~Camera();

		virtual bool Setup(void* data) override;
		virtual void Shutdown() override;

	private:
		Transform cameraToWorld;
	};
}