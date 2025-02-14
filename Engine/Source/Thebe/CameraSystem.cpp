#include "CameraSystem.h"
#include "Thebe/Math/Quaternion.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "Thebe/Log.h"

using namespace Thebe;

//------------------------------- CameraSystem -------------------------------

CameraSystem::CameraSystem()
{
	this->doBlending = true;
	this->boundToController = false;
	this->translationalBlendRate = 50.0;
	this->rotationalBlendRate = 1.0;
}

/*virtual*/ CameraSystem::~CameraSystem()
{
}

void CameraSystem::SetActiveController(const std::string& name)
{
	if (this->activeControllerStack.size() == 0)
		this->PushActiveController(name);
	else
	{
		int i = (int)this->activeControllerStack.size() - 1;
		this->activeControllerStack[i] = name;
	}

	this->boundToController = false;
}

std::string CameraSystem::GetActiveController()
{
	if (this->activeControllerStack.size() == 0)
		return "";

	int i = (int)this->activeControllerStack.size() - 1;
	return this->activeControllerStack[i];
}

void CameraSystem::PushActiveController(const std::string& name)
{
	this->activeControllerStack.push_back(name);
	this->boundToController = false;
}

void CameraSystem::PopActiveController()
{
	this->activeControllerStack.pop_back();
	this->boundToController = false;
}

void CameraSystem::SetCamera(Camera* camera)
{
	this->camera = camera;
}

Camera* CameraSystem::GetCamera()
{
	return this->camera;
}

CameraController* CameraSystem::GetControllerByName(const std::string& name)
{
	auto pair = this->controllerMap.find(name);
	if (pair == this->controllerMap.end())
		return nullptr;

	return pair->second;
}

void CameraSystem::Update(double deltaTimeSeconds)
{
	if (!this->camera)
		return;

	CameraController* controller = this->GetControllerByName(this->GetActiveController());
	if (!controller)
		return;

	const Transform& currentCameraToWorld = this->camera->GetCameraToWorldTransform();
	Transform desiredCameraToWorld = currentCameraToWorld;
	controller->ControlCamera(desiredCameraToWorld, deltaTimeSeconds);
	
	Transform cameraToWorld;
	if (this->boundToController)
		cameraToWorld = desiredCameraToWorld;
	else
	{
		double translationalStep = this->translationalBlendRate * deltaTimeSeconds;
		double rotationalStep = this->rotationalBlendRate * deltaTimeSeconds;
		if (!cameraToWorld.MoveTo(currentCameraToWorld, desiredCameraToWorld, translationalStep, rotationalStep))
			this->boundToController = true;
	}

	this->camera->SetCameraToWorldTransform(cameraToWorld);
}

void CameraSystem::AddController(const std::string& name, CameraController* controller)
{
	this->controllerMap.erase(name);
	this->controllerMap.insert(std::pair(name, controller));
}

void CameraSystem::RemoveController(const std::string& name)
{
	this->controllerMap.erase(name);
}

void CameraSystem::ClearAllControllers()
{
	this->controllerMap.clear();
}

//------------------------------- CameraController -------------------------------

CameraController::CameraController()
{
}

/*virtual*/ CameraController::~CameraController()
{
}

/*virtual*/ void CameraController::ControlCamera(Transform& cameraToWorld, double deltaTimeSeconds)
{
	cameraToWorld.SetIdentity();
}

//------------------------------- FreeCam -------------------------------

FreeCam::FreeCam()
{
	this->strafeMode = StrafeMode::XZ_PLANE;
	this->moveSpeed = 40.0;
	this->rotationSpeed = 1.5;
}

/*virtual*/ FreeCam::~FreeCam()
{
}

void FreeCam::SetXBoxController(XBoxController* controller)
{
	if (this->controller)
		this->controller->RemoveButtonHandler(this);

	this->controller = controller;

	if (this->controller)
		this->controller->AddButtonHandler(this);
}

/*virtual*/ void FreeCam::ControlCamera(Transform& cameraToWorld, double deltaTimeSeconds)
{
	// This prevents a large jump if we were just stopped in the debugger for a time.
	if (deltaTimeSeconds > 1.0)
		return;

	if (!this->controller)
		return;

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

	Vector2 leftJoyStick = this->controller->GetAnalogJoyStick(XINPUT_GAMEPAD_LEFT_THUMB);
	Vector3 eyeDelta = leftJoyStick.x * strafeAxisX + leftJoyStick.y * strafeAxisY;
	cameraToWorld.translation += eyeDelta * this->moveSpeed * deltaTimeSeconds;

	Vector2 rightJoyStick = this->controller->GetAnalogJoyStick(XINPUT_GAMEPAD_RIGHT_THUMB);

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

//------------------------------- FollowCam -------------------------------

FollowCam::FollowCam()
{
	this->followDistance = 20.0;
	this->heightBounds.A = std::numeric_limits<double>::max();
	this->heightBounds.B = -std::numeric_limits<double>::max();
	this->subjectHandle = THEBE_INVALID_REF_HANDLE;
}

/*virtual*/ FollowCam::~FollowCam()
{
}

/*virtual*/ void FollowCam::ControlCamera(Transform& cameraToWorld, double deltaTimeSeconds)
{
	Reference<ReferenceCounted> ref;
	HandleManager::Get()->GetObjectFromHandle(this->subjectHandle, ref);
	if (!ref)
		return;

	auto space = dynamic_cast<Space*>(ref.Get());
	auto collisionObject = dynamic_cast<CollisionObject*>(ref.Get());
	
	Vector3 subjectLocation(0.0, 0.0, 0.0);
	if (space)
		subjectLocation = space->GetObjectToWorldTransoform().translation;
	else if (collisionObject)
		subjectLocation = collisionObject->GetObjectToWorld().translation;
	
	Vector3 cameraEye = cameraToWorld.translation;
	Vector3 delta = cameraEye - subjectLocation;
	double length = delta.Length();
	delta *= this->followDistance / length;
	cameraEye = subjectLocation + delta;
	cameraEye.y = this->heightBounds.Clamp(cameraEye.y);
	cameraToWorld.LookAt(cameraEye, subjectLocation, Vector3::YAxis());
}

void FollowCam::SetHeightBounds(const Thebe::Interval& heightBounds)
{
	this->heightBounds = heightBounds;
}

const Thebe::Interval& FollowCam::GetHeightBounds() const
{
	return this->heightBounds;
}

void FollowCam::SetSubject(RefHandle subjectHandle)
{
	this->subjectHandle = subjectHandle;
}

void FollowCam::SetFollowDistance(double followDistance)
{
	this->followDistance = followDistance;
}

double FollowCam::GetFollowDistance() const
{
	return this->followDistance;
}