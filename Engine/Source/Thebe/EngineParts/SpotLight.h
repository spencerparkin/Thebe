#pragma once

#include "Thebe/EngineParts/Light.h"
#include "Thebe/EngineParts/Camera.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API SpotLight : public Light
	{
	public:
		SpotLight();
		virtual ~SpotLight();

		virtual Camera* GetCamera() override;
		virtual bool SetShaderParameters(ConstantsBuffer* constantsBuffer) override;
		virtual bool SetLightToWorldTransform(const Transform& lightToWorld) override;

		void SetConeAngle(double coneAngle);
		double GetConeAngle() const;

	private:
		Reference<PerspectiveCamera> camera;
		double coneAngle;
	};
}