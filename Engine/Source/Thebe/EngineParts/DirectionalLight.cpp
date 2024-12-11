#include "Thebe/EngineParts/DirectionalLight.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"

using namespace Thebe;

DirectionalLight::DirectionalLight()
{
	this->camera.Set(new OrthographicCamera());
}

/*virtual*/ DirectionalLight::~DirectionalLight()
{
}

/*virtual*/ Camera* DirectionalLight::GetCamera()
{
	return this->camera.Get();
}

/*virtual*/ bool DirectionalLight::SetShaderParameters(ConstantsBuffer* constantsBuffer)
{
	if (!Light::SetShaderParameters(constantsBuffer))
		return false;

	if (constantsBuffer->GetParameterType("unitWorldLightDir") != Shader::Parameter::FLOAT3)
		return false;

	// Note that here, for the convenience of the shader, the given light direction vector points toward (not away from) the light source.
	const Transform& cameraToWorld = this->camera->GetCameraToWorldTransform();
	Vector3 worldUnitLightDirection = cameraToWorld.matrix.GetColumnVector(2);
	constantsBuffer->SetParameter("unitWorldLightDir", worldUnitLightDirection);

	return true;
}

/*virtual*/ bool DirectionalLight::SetLightToWorldTransform(const Transform& lightToWorld)
{
	this->camera->SetCameraToWorldTransform(lightToWorld);
	return true;
}