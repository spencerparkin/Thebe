#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/ConstantsBuffer.h"

using namespace Thebe;

//---------------------------------- Camera ----------------------------------

Camera::Camera()
{
}

/*virtual*/ Camera::~Camera()
{
}

/*virtual*/ bool Camera::Setup()
{
	return true;
}

/*virtual*/ void Camera::Shutdown()
{
}

void Camera::SetCameraToWorldTransform(const Transform& cameraToWorld)
{
	this->cameraToWorld = cameraToWorld;
	this->worldToCamera.Invert(this->cameraToWorld);
}

const Transform& Camera::GetCameraToWorldTransform() const
{
	return this->cameraToWorld;
}

void Camera::SetCameraToProjectionMatrix(const Matrix4x4& cameraToProjection)
{
	this->cameraToProjection = cameraToProjection;
}

const Matrix4x4& Camera::GetCameraToProjectionMatrix() const
{
	return this->cameraToProjection;
}

const Transform& Camera::GetWorldToCameraTransform() const
{
	return this->worldToCamera;
}

/*virtual*/ bool Camera::CanSee(const RenderObject* renderObject) const
{
	return true;
}

/*virtual*/ void Camera::UpdateProjection(double aspectRatio)
{
}

/*virtual*/ bool Camera::SetShaderParameters(ConstantsBuffer* constantsBuffer)
{
	if (constantsBuffer->GetParameterType("unitWorldCamDir") != Shader::Parameter::FLOAT3)
		return false;

	Vector3 unitWorldCameraDirection = this->cameraToWorld.matrix.GetColumnVector(2);
	constantsBuffer->SetParameter("unitWorldCamDir", unitWorldCameraDirection);
	return true;
}

//---------------------------------- PerspectiveCamera ----------------------------------

PerspectiveCamera::PerspectiveCamera()
{
}

/*virtual*/ PerspectiveCamera::~PerspectiveCamera()
{
}

/*virtual*/ void PerspectiveCamera::UpdateProjection(double aspectRatio)
{
	this->frustum.SetFromAspectRatio(aspectRatio, this->frustum.hfovi, this->frustum.nearClip, this->frustum.farClip);
	this->frustum.GetToProjectionMatrix(this->cameraToProjection);
}

/*virtual*/ bool PerspectiveCamera::CanSee(const RenderObject* renderObject) const
{
	// TODO: Write this.  Check the given render object's bounding box or sphere or something against our frustum.
	return true;
}

Frustum& PerspectiveCamera::GetFrustum()
{
	return this->frustum;
}

//---------------------------------- OrthographicCamera ----------------------------------

OrthographicCamera::OrthographicCamera()
{
	this->params.width = 10.0;
	this->params.height = 10.0;
	this->params.nearClip = 0.001;
	this->params.farClip = 10000.0;
}

/*virtual*/ OrthographicCamera::~OrthographicCamera()
{
}

/*virtual*/ void OrthographicCamera::UpdateProjection(double aspectRatio)
{
	Params adjustedParams = this->params;

	if (aspectRatio != 0.0)
	{
		double currentAspectRatio = adjustedParams.width / adjustedParams.height;

		if (aspectRatio > currentAspectRatio)
			adjustedParams.width += adjustedParams.height * aspectRatio - adjustedParams.width;
		else if (aspectRatio < currentAspectRatio)
			adjustedParams.height += adjustedParams.width / aspectRatio - adjustedParams.height;
	}

	this->cameraToProjection.SetOrthographicProjection(adjustedParams.width, adjustedParams.height, adjustedParams.nearClip, adjustedParams.farClip);
}

/*virtual*/ bool OrthographicCamera::CanSee(const RenderObject* renderObject) const
{
	// TODO: Write this.
	return true;
}

OrthographicCamera::Params& OrthographicCamera::GetParams()
{
	return this->params;
}