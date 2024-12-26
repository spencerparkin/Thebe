#include "Thebe/FreeCam.h"
#include "Thebe/EngineParts/Camera.h"
#include "Thebe/Math/Quaternion.h"
#include "Thebe/Log.h"

using namespace Thebe;

FreeCam::FreeCam() : controller(0)
{
	this->controller.AddButtonHandler(this);
	this->strafeMode = StrafeMode::XZ_PLANE;
	this->moveSpeed = 40.0;
	this->rotationSpeed = 1.5;
}

/*virtual*/ FreeCam::~FreeCam()
{
}

void FreeCam::SetCamera(Camera* camera)
{
	this->camera = camera;
}

Camera* FreeCam::GetCamera()
{
	return this->camera;
}

XBoxController* FreeCam::GetController()
{
	return &this->controller;
}

/*virtual*/ void FreeCam::Update(double deltaTimeSeconds)
{
	// This prevents a large jump if we were just stopped in the debugger for a time.
	if (deltaTimeSeconds > 1.0)
		return;

	if (!this->camera.Get())
		return;

	this->controller.Update();

	Transform cameraToWorld = this->camera->GetCameraToWorldTransform();

	Vector3 xAxis, yAxis, zAxis;
	cameraToWorld.matrix.GetColumnVectors(xAxis, yAxis, zAxis);

	Vector3 strafeAxisX, strafeAxisY;

	strafeAxisX = xAxis;
	switch (this->strafeMode)
	{
	case StrafeMode::XZ_PLANE:
		strafeAxisY = -zAxis;
		break;
	case StrafeMode::XY_PLANE:
		strafeAxisY = yAxis;
		break;
	}

	Vector2 leftJoyStick = this->controller.GetAnalogJoyStick(XINPUT_GAMEPAD_LEFT_THUMB);
	Vector3 eyeDelta = leftJoyStick.x * strafeAxisX + leftJoyStick.y * strafeAxisY;
	cameraToWorld.translation += eyeDelta * this->moveSpeed * deltaTimeSeconds;

	Vector2 rightJoyStick = this->controller.GetAnalogJoyStick(XINPUT_GAMEPAD_RIGHT_THUMB);

	double xAxisRotationAngle, yAxisRotationAngle;
	xAxisRotationAngle = rightJoyStick.y * this->rotationSpeed * deltaTimeSeconds;
	yAxisRotationAngle = -rightJoyStick.x * this->rotationSpeed * deltaTimeSeconds;

	Quaternion xAxisRotation, yAxisRotation;
	xAxisRotation.SetFromAxisAngle(xAxis, xAxisRotationAngle);
	yAxisRotation.SetFromAxisAngle(yAxis, yAxisRotationAngle);

	Matrix3x3 rotationMatrix;
	rotationMatrix.SetFromQuat(xAxisRotation * yAxisRotation);
	cameraToWorld.matrix = rotationMatrix * cameraToWorld.matrix;

	xAxis = cameraToWorld.matrix.GetColumnVector(0);
	Vector3 upVector(0.0, 1.0, 0.0);
	Vector3 desiredXAxis = xAxis.RejectedFrom(upVector).Normalized();
	rotationMatrix.MakeRotation(xAxis, desiredXAxis);
	cameraToWorld.matrix = rotationMatrix * cameraToWorld.matrix;
	
	cameraToWorld.matrix.Orthonormalized(THEBE_AXIS_FLAG_X);

	this->camera->SetCameraToWorldTransform(cameraToWorld);
}

/*virtual*/ void FreeCam::OnButtonPressed(DWORD button)
{
	switch (button)
	{
		case XINPUT_GAMEPAD_DPAD_UP:
		{
			this->moveSpeed *= 2.0;
			if (this->moveSpeed > THEBE_FREE_CAM_MAX_MOVE_SPEED)
				this->moveSpeed = THEBE_FREE_CAM_MAX_MOVE_SPEED;
			THEBE_LOG("Move speed increased to %f.", this->moveSpeed);
			break;
		}
		case XINPUT_GAMEPAD_DPAD_DOWN:
		{
			this->moveSpeed /= 2.0;
			if (this->moveSpeed < THEBE_FREE_CAM_MIN_MOVE_SPEED)
				this->moveSpeed = THEBE_FREE_CAM_MIN_MOVE_SPEED;
			THEBE_LOG("Move speed decreased to %f.", this->moveSpeed);
			break;
		}
		case XINPUT_GAMEPAD_RIGHT_SHOULDER:
		{
			switch (this->strafeMode)
			{
				case StrafeMode::XZ_PLANE:
				{
					this->strafeMode = StrafeMode::XY_PLANE;
					THEBE_LOG("Strafing now in XY plane of camera.");
					break;
				}
				case StrafeMode::XY_PLANE:
				{
					this->strafeMode = StrafeMode::XZ_PLANE;
					THEBE_LOG("Strafing now in XZ plane of camera.");
					break;
				}
			}
			break;
		}
	}
}

/*virtual*/ void FreeCam::OnButtonReleased(DWORD button)
{
}