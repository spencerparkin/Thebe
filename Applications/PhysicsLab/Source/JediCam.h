#pragma once

#include "Thebe/FreeCam.h"
#include "Thebe/EngineParts/PhysicsObject.h"

class JediCam : public Thebe::FreeCam
{
public:
	JediCam();
	virtual ~JediCam();

	virtual void Update(double deltaTimeSeconds) override;
	virtual void OnButtonPressed(DWORD button) override;

	void AddObject(Thebe::PhysicsObject* moveObject);

private:
	enum Mode
	{
		FREECAM,
		INFLUENCE_OBJECT
	};

	Mode mode;

	void UpdateObjectColors();

	std::vector<Thebe::Reference<Thebe::PhysicsObject>> objectArray;
	UINT objectIndex;
};