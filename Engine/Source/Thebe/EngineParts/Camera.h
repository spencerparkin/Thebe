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
		const Transform& GetCameraToWorldTransform() const;
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

		virtual void UpdateProjection(double aspectRatio) override;
		virtual bool CanSee(const RenderObject* renderObject) const override;

		Frustum& GetFrustum();

	private:
		Frustum frustum;
	};

	/**
	 * 
	 */
	class THEBE_API OrthographicCamera : public Camera
	{
	public:
		OrthographicCamera();
		virtual ~OrthographicCamera();

		virtual void UpdateProjection(double aspectRatio) override;
		virtual bool CanSee(const RenderObject* renderObject) const override;

		struct Params
		{
			double width;
			double height;
			double nearClip;
			double farClip;
		};

		Params& GetParams();

	private:
		Params params;
	};
}