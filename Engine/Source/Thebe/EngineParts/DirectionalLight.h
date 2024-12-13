#pragma once

#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/Camera.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API DirectionalLight : public Light
	{
	public:
		DirectionalLight();
		virtual ~DirectionalLight();

		virtual bool Setup() override;
		virtual Camera* GetCamera() override;
		virtual bool SetShaderParameters(ConstantsBuffer* constantsBuffer) override;
		virtual bool SetLightToWorldTransform(const Transform& lightToWorld) override;

	private:
		Reference<OrthographicCamera> camera;
	};
}