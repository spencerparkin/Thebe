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

	// TODO: Probably need to send in the near clip and far clip parameters instead of this.
	const OrthographicCamera::Params& params = this->camera->GetParams();
	double shadowVolumeExtent = params.farClip - params.nearClip;
	if (!constantsBuffer->SetParameter("shadowVolumeExtent", shadowVolumeExtent))
		return false;

	Vector3 xAxis = cameraToWorld.matrix.GetColumnVector(0);
	Vector3 yAxis = cameraToWorld.matrix.GetColumnVector(1);

	Matrix3x3 matrixA;
	matrixA.ele[0][0] = xAxis.x;
	matrixA.ele[0][1] = xAxis.y;
	matrixA.ele[0][2] = xAxis.z;
	matrixA.ele[1][0] = yAxis.x;
	matrixA.ele[1][1] = yAxis.y;
	matrixA.ele[1][2] = yAxis.z;

	Matrix3x3 matrixB;
	matrixB.ele[0][0] = 1.0 / params.width;
	matrixB.ele[1][1] = 1.0 / params.height;
	matrixB.ele[0][2] = 0.5;
	matrixB.ele[1][2] = 0.5;

	Matrix3x3 matrixC;
	matrixC.ele[1][1] = -1.0;
	matrixC.ele[1][2] = 1.0;

	Matrix3x3 shadowMatrix = matrixC * matrixB * matrixA;
	if (!constantsBuffer->SetParameter("shadowMatrix", shadowMatrix))
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