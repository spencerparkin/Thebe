#pragma once

#include "Thebe/CameraSystem.h"
#include "Thebe/Math/SphericalCoords.h"

/**
 * 
 */
class BoardCam : public Thebe::CameraController
{
public:
	BoardCam();
	virtual ~BoardCam();

	virtual void ControlCamera(Thebe::Transform& cameraToWorld, double deltaTimeSeconds);

	void ApplyStrafe(const Thebe::Vector3& strafeVector);
	void ApplyZoom(double zoomDelta);
	void ApplyPivot(double pivotAngleDelta);
	void ApplyTilt(double tiltAngleDelta);

	struct StaticParams
	{
		Thebe::Vector3 diskCenter;
		double diskRadius;
		Thebe::Interval zoomInterval;
		Thebe::Interval tiltRange;
	};

	struct DynamicParams
	{
		Thebe::Vector3 focalPoint;
		Thebe::SphericalCoords relativeEyePoint;
	};

	void SetStaticParams(const StaticParams& staticParams);
	const StaticParams& GetStaticParams() const;

	void SetDynamicParams(const DynamicParams& dynamicParams);
	const DynamicParams& GetDynamicParams() const;

private:
	StaticParams staticParams;
	DynamicParams dynamicParams;
};