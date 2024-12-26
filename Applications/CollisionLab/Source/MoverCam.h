#pragma once

#include "Thebe/FreeCam.h"
#include "Thebe/EngineParts/CollisionObject.h"

class MoverCam : public Thebe::FreeCam
{
public:
	MoverCam();
	virtual ~MoverCam();

	virtual void Update(double deltaTimeSeconds) override;
	virtual void OnButtonPressed(DWORD button) override;

	void AddMoveObject(Thebe::CollisionObject* moveObject);

private:
	enum Mode
	{
		MOVE_FREECAM,
		MOVE_OBJECT
	};

	Mode mode;

	void UpdateMoveObjectColors();

	std::vector<Thebe::Reference<Thebe::CollisionObject>> moveObjectArray;
	UINT moveObjectIndex;
};