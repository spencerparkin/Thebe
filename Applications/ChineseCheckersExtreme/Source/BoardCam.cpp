#include "BoardCam.h"
#include "Thebe/Math/Angle.h"

using namespace Thebe;

BoardCam::BoardCam()
{
	this->staticParams.diskCenter.SetComponents(0.0, 0.0, 0.0);
	this->staticParams.diskRadius = 75.0;
	this->staticParams.zoomInterval.A = 2.0;
	this->staticParams.zoomInterval.B = 200.0;
	this->staticParams.tiltRange.A = THEBE_PI / 256.0;
	this->staticParams.tiltRange.B = THEBE_PI / 2.0 - THEBE_PI / 256.0;

	this->dynamicParams.focalPoint = this->staticParams.diskCenter;
	this->dynamicParams.relativeEyePoint.SetFromVector(Vector3(100.0, 100.0, 100.0));
}

/*virtual*/ BoardCam::~BoardCam()
{
}

/*virtual*/ void BoardCam::ControlCamera(Thebe::Transform& cameraToWorld, double deltaTimeSeconds)
{
	Vector3 delta = this->dynamicParams.relativeEyePoint.GetToVector();
	Vector3 eyePoint = this->dynamicParams.focalPoint + delta;

	cameraToWorld.LookAt(eyePoint, this->dynamicParams.focalPoint, Vector3::YAxis());
}

void BoardCam::ApplyStrafe(const Thebe::Vector3& strafeVector)
{
	this->dynamicParams.focalPoint += strafeVector;
	this->dynamicParams.focalPoint.y = 0.0;

	Vector3 vector = this->dynamicParams.focalPoint - this->staticParams.diskCenter;
	double distance = vector.Length();
	if (distance > this->staticParams.diskRadius)
	{
		vector *= this->staticParams.diskRadius / distance;
		this->dynamicParams.focalPoint = this->staticParams.diskCenter + vector;
	}
}

void BoardCam::ApplyZoom(double zoomDelta)
{
	this->dynamicParams.relativeEyePoint.radius += zoomDelta;
	this->dynamicParams.relativeEyePoint.radius = this->staticParams.zoomInterval.Clamp(this->dynamicParams.relativeEyePoint.radius);
}

void BoardCam::ApplyPivot(double pivotAngleDelta)
{
	this->dynamicParams.relativeEyePoint.longitudeAngle += pivotAngleDelta;
	this->dynamicParams.relativeEyePoint.longitudeAngle = Angle::Mod2Pi(this->dynamicParams.relativeEyePoint.longitudeAngle);
}

void BoardCam::ApplyTilt(double tiltAngleDelta)
{
	this->dynamicParams.relativeEyePoint.latitudeAngle += tiltAngleDelta;
	this->dynamicParams.relativeEyePoint.latitudeAngle = this->staticParams.tiltRange.Clamp(this->dynamicParams.relativeEyePoint.latitudeAngle);
}

void BoardCam::SetStaticParams(const StaticParams& staticParams)
{
	this->staticParams = staticParams;
}

const BoardCam::StaticParams& BoardCam::GetStaticParams() const
{
	return this->staticParams;
}

void BoardCam::SetDynamicParams(const DynamicParams& dynamicParams)
{
	this->dynamicParams = dynamicParams;
}

const BoardCam::DynamicParams& BoardCam::GetDynamicParams() const
{
	return this->dynamicParams;
}