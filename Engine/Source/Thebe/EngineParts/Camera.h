#pragma once

#include "Thebe/EnginePart.h"
#include "Thebe/Math/Transform.h"
#include "Thebe/Math/Matrix4x4.h"
#include "Thebe/Math/Frustum.h"

namespace Thebe
{
	class RenderObject;

	/**
	 * 
	 */
	class THEBE_API Camera : public EnginePart
	{
	public:
		Camera();
		virtual ~Camera();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual void UpdateProjection(double aspectRatio);
		virtual bool CanSee(const RenderObject* renderObject) const;

		void SetCameraToWorldTransform(const Transform& cameraToWorld);
		void SetCameraToProjectionMatrix(const Matrix4x4& cameraToProjection);
		const Matrix4x4& GetCameraToProjectionMatrix() const;
		const Transform& GetWorldToCameraTransform() const;
		Frustum& GetFrustum();

	protected:
		Transform cameraToWorld;
		Transform worldToCamera;
		Matrix4x4 cameraToProjection;
	};

	/**
	 * 
	 */
	class THEBE_API PerspectiveCamera : public Camera
	{
	public:
		PerspectiveCamera();
		virtual ~PerspectiveCamera();

		virtual void UpdateProjection(double aspectRatio);
		virtual bool CanSee(const RenderObject* renderObject) const;

		Frustum& GetFrustum();

	private:
		Frustum frustum;
	};
}