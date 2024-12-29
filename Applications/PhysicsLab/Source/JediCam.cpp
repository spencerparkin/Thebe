#include "JediCam.h"
#include "Thebe/EngineParts/Camera.h"

JediCam::JediCam()
{
	this->mode = Mode::FREECAM;
	this->objectIndex = 0;
}

/*virtual*/ JediCam::~JediCam()
{
}

/*virtual*/ void JediCam::Update(double deltaTimeSeconds)
{
	using namespace Thebe;

	switch (this->mode)
	{
		case Mode::FREECAM:
		{
			FreeCam::Update(deltaTimeSeconds);
			break;
		}
		case Mode::INFLUENCE_OBJECT:
		{
			this->controller.Update();

			if (this->objectIndex < this->objectArray.size())
			{
				PhysicsObject* object = this->objectArray[this->objectIndex];

				Transform cameraToWorld = this->camera->GetCameraToWorldTransform();

				Vector3 xAxis, yAxis, zAxis;
				cameraToWorld.matrix.GetColumnVectors(xAxis, yAxis, zAxis);

				Vector2 leftJoyStick = this->controller.GetAnalogJoyStick(XINPUT_GAMEPAD_LEFT_THUMB);
				Vector2 rightJoyStick = this->controller.GetAnalogJoyStick(XINPUT_GAMEPAD_RIGHT_THUMB);

				double forceStrength = 1.0;
				Vector3 force(0.0, 0.0, 0.0);
				force += xAxis * leftJoyStick.x * forceStrength;
				force += yAxis * leftJoyStick.y * forceStrength;
				object->SetExternalForce("jedi", force);

				double torqueStrength = 1.0;
				Vector3 torque(0.0, 0.0, 0.0);
				torque += xAxis * leftJoyStick.y * torqueStrength;
				torque += yAxis * leftJoyStick.x * torqueStrength;
				object->SetExternalTorque("jedi", torque);
			}

			break;
		}
	}
}

/*virtual*/ void JediCam::OnButtonPressed(DWORD button)
{
	switch (this->mode)
	{
		case Mode::FREECAM:
		{
			if (button == XINPUT_GAMEPAD_X)
			{
				this->mode = Mode::INFLUENCE_OBJECT;
				this->UpdateObjectColors();
				break;
			}

			FreeCam::OnButtonPressed(button);
			break;
		}
		case Mode::INFLUENCE_OBJECT:
		{
			switch (button)
			{
				case XINPUT_GAMEPAD_X:
				{
					this->mode = Mode::FREECAM;
					break;
				}
				case XINPUT_GAMEPAD_Y:
				{
					this->objectIndex = (this->objectIndex + 1) % this->objectArray.size();
					this->UpdateObjectColors();
					break;
				}
			}

			break;
		}
	}
}

void JediCam::AddObject(Thebe::PhysicsObject* object)
{
	this->objectArray.push_back(object);
}

void JediCam::UpdateObjectColors()
{
	using namespace Thebe;

	for (UINT i = 0; i < (UINT)this->objectArray.size(); i++)
	{
		PhysicsObject* object = this->objectArray[i];
		if (i == this->objectIndex)
			object->GetCollisionObject()->SetDebugColor(Vector3(0.0, 1.0, 0.0));
		else
			object->GetCollisionObject()->SetDebugColor(Vector3(1.0, 1.0, 1.0));
	}
}