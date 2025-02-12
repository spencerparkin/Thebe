#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Transform.h"

namespace Thebe
{
	class Camera;
	class ConstantsBuffer;

	/**
	 * This is the base class for any type of light we want for lighting a scene.
	 */
	class THEBE_API Light : public EnginePart
	{
	public:
		Light();
		virtual ~Light();

		/**
		 * This is needed for the shadow pass.
		 */
		virtual Camera* GetCamera();

		/**
		 * This is called in preparation for drawing a lit render object.
		 */
		virtual bool SetShaderParameters(ConstantsBuffer* constantsBuffer);

		/**
		 * All or part of this transform may be applicable to the light source.
		 */
		virtual bool SetLightToWorldTransform(const Transform& lightToWorld);

		void SetLightColor(const Vector3& lightColor);
		const Vector3& GetLightColor() const;

		void SetAmbientLightColor(const Vector3& ambientLightColor);
		const Vector3& GetAmbientLightColor() const;

		void SetLightIntensity(double lightIntensity);
		double GetLightIntensity() const;

	protected:
		Vector3 lightColor;
		Vector3 ambientLightColor;
		double lightIntensity;
	};
}