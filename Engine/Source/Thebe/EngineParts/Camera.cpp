#include "Thebe/EngineParts/Camera.h"

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