#include "Thebe/EngineParts/SpotLight.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"

using namespace Thebe;

SpotLight::SpotLight()
{
	this->coneAngle = M_PI / 6.0;
	this->camera.Set(new PerspectiveCamera());
}

/*virtual*/ SpotLight::~SpotLight()
{
}

/*virtual*/ Camera* SpotLight::GetCamera()
{
	return this->camera.Get();
}

void SpotLight::SetConeAngle(double coneAngle)
{
	this->coneAngle = coneAngle;
}

double SpotLight::GetConeAngle() const
{
	return this->coneAngle;
}

/*virtual*/ bool SpotLight::SetShaderParameters(ConstantsBuffer* constantsBuffer)
{
	// TODO: May want to come back and revise all this to account for the possibility of having multiple spot-lights.
	//       For now, whenever we draw an object lit by a spot-light, there is just one spot-light doing the lighting.

	if (!Light::SetShaderParameters(constantsBuffer))
		return false;

	if (constantsBuffer->GetParameterType("worldLightPosition") != Shader::Parameter::FLOAT3)
		return false;

	const Transform& cameraToWorld = this->camera->GetCameraToWorldTransform();
	constantsBuffer->SetParameter("worldLightPosition", cameraToWorld.translation);

	if (constantsBuffer->GetParameterType("worldUnitLightDirection") != Shader::Parameter::FLOAT3)
		return false;

	// Notice that for convenience in the shader, this points toward (not away) from the light source.
	Vector3 worldUnitLightDirection = cameraToWorld.matrix.GetColumnVector(2);
	constantsBuffer->SetParameter("worldUnitLightDirection", worldUnitLightDirection);

	if (constantsBuffer->GetParameterType("lightConeAngle") != Shader::Parameter::FLOAT)
		return false;

	constantsBuffer->SetParameter("lightConeAngle", this->coneAngle);

	return true;
}

/*virtual*/ bool SpotLight::SetLightToWorldTransform(const Transform& lightToWorld)
{
	this->camera->SetCameraToWorldTransform(lightToWorld);
	return true;
}