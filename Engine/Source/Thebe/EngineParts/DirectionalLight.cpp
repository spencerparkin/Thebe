#include "Thebe/EngineParts/DirectionalLight.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"
#include "Thebe/Math/Matrix3x3.h"

using namespace Thebe;

DirectionalLight::DirectionalLight()
{
}

/*virtual*/ DirectionalLight::~DirectionalLight()
{
}

/*virtual*/ bool DirectionalLight::Setup()
{
	this->camera.Set(new OrthographicCamera());
	OrthographicCamera::Params& params = this->camera->GetParams();
	params.width = 200.0;
	params.height = 200.0;
	return true;
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

	// Yes, our light source is conceptually infinitely far away, but for shadow
	// calculations, we need to give the light a position along the light ray.
	if (!constantsBuffer->SetParameter("worldLightPos", cameraToWorld.translation))
		return false;

	const OrthographicCamera::Params& params = this->camera->GetParams();
	
	if (!constantsBuffer->SetParameter("shadowNearClip", params.nearClip))
		return false;
	
	if (!constantsBuffer->SetParameter("shadowFarClip", params.farClip))
		return false;

	if (!constantsBuffer->SetParameter("shadowWidth", params.width))
		return false;

	if (!constantsBuffer->SetParameter("shadowHeight", params.height))
		return false;

	if (!constantsBuffer->SetParameter("worldLightXAxis", cameraToWorld.matrix.GetColumnVector(0)))
		return false;

	if (!constantsBuffer->SetParameter("worldLightYAxis", cameraToWorld.matrix.GetColumnVector(1)))
		return false;

	return true;
}

/*virtual*/ bool DirectionalLight::SetLightToWorldTransform(const Transform& lightToWorld)
{
	if (!this->camera.Get())
		return false;

	this->camera->SetCameraToWorldTransform(lightToWorld);
	return true;
}