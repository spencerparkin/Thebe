#include "MoverCam.h"
#include "Thebe/EngineParts/Camera.h"

MoverCam::MoverCam()
{
	this->mode = Mode::MOVE_FREECAM;
	this->moveObjectIndex = 0;
}

/*virtual*/ MoverCam::~MoverCam()
{
}

void MoverCam::AddMoveObject(Thebe::CollisionObject* moveObject)
{
	this->moveObjectArray.push_back(moveObject);
}

/*virtual*/ void MoverCam::Update(double deltaTimeSeconds)
{
	using namespace Thebe;

	switch (this->mode)
	{
		case Mode::MOVE_FREECAM:
		{
			FreeCam::Update(deltaTimeSeconds);
			break;
		}
		case Mode::MOVE_OBJECT:
		{
			this->controller.Update();

			if (this->moveObjectIndex < this->moveObjectArray.size())
			{
				CollisionObject* moveObject = this->moveObjectArray[this->moveObjectIndex];

				Transform cameraToWorld = this->camera->GetCameraToWorldTransform();

				Vector3 xAxis, yAxis, zAxis;
				cameraToWorld.matrix.GetColumnVectors(xAxis, yAxis, zAxis);

				Vector2 leftJoyStick = this->controller.GetAnalogJoyStick(XINPUT_GAMEPAD_LEFT_THUMB);
				Vector2 rightJoyStick = this->controller.GetAnalogJoyStick(XINPUT_GAMEPAD_RIGHT_THUMB);

				Transform objectToWorld = moveObject->GetObjectToWorld();
				
				Vector3 delta(0.0, 0.0, 0.0);
				static double moveSensativity = 0.01;
				delta += leftJoyStick.x * moveSensativity * xAxis;
				delta += leftJoyStick.y * moveSensativity * yAxis;
				objectToWorld.translation += delta;

				double rotateSensativity = 0.01;
				Matrix3x3 xAxisRotation, yAxisRotation;
				double xAngleDelta = -rightJoyStick.y * rotateSensativity;
				double yAngleDelta = rightJoyStick.x * rotateSensativity;
				xAxisRotation.SetFromAxisAngle(xAxis, xAngleDelta);
				yAxisRotation.SetFromAxisAngle(yAxis, yAngleDelta);
				objectToWorld.matrix = (xAxisRotation * yAxisRotation * objectToWorld.matrix).Orthonormalized(THEBE_AXIS_FLAG_X);

				moveObject->SetObjectToWorld(objectToWorld);
			}

			break;
		}
	}
}

/*virtual*/ void MoverCam::OnButtonPressed(DWORD button)
{
	switch (this->mode)
	{
		case Mode::MOVE_FREECAM:
		{
			if (button == XINPUT_GAMEPAD_X)
			{
				this->mode = Mode::MOVE_OBJECT;
				this->UpdateMoveObjectColors();
				break;
			}
			
			FreeCam::OnButtonPressed(button);
			break;
		}
		case Mode::MOVE_OBJECT:
		{
			switch (button)
			{
				case XINPUT_GAMEPAD_X:
				{
					this->mode = Mode::MOVE_FREECAM;
					break;
				}
				case XINPUT_GAMEPAD_Y:
				{
					this->moveObjectIndex = (this->moveObjectIndex + 1) % this->moveObjectArray.size();
					this->UpdateMoveObjectColors();
					break;
				}
			}

			break;
		}
	}
}

void MoverCam::UpdateMoveObjectColors()
{
	using namespace Thebe;

	for (UINT i = 0; i < (UINT)this->moveObjectArray.size(); i++)
	{
		CollisionObject* moveObject = this->moveObjectArray[i];
		if (i == this->moveObjectIndex)
			moveObject->SetDebugColor(Vector3(1.0, 0.0, 0.0));
		else
			moveObject->SetDebugColor(Vector3(1.0, 1.0, 1.0));
	}
}