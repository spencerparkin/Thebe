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

	// For convenience in the shader, this points toward the light, not away from it.
	const Transform& cameraToWorld = this->camera->GetCameraToWorldTransform();
	Vector3 worldLightDir = cameraToWorld.matrix.GetColumnVector(2);
	if (!constantsBuffer->SetParameter("worldLightDir", worldLightDir))
		return false;

	if (!constantsBuffer->SetParameter("lightDistanceInfinite", 1.0))
		return false;

	if (constantsBuffer->GetParameterType("worldLightPos") == Shader::Parameter::FLOAT3)
		constantsBuffer->SetParameter("worldLightPos", Vector3(0.0, 0.0, 0.0));

	return true;
}

/*virtual*/ bool DirectionalLight::SetLightToWorldTransform(const Transform& lightToWorld)
{
	this->camera->SetCameraToWorldTransform(lightToWorld);
	return true;
}