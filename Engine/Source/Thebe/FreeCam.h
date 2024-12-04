#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/XBoxController.h"

#define THEBE_FREE_CAM_MAX_MOVE_SPEED		100.0
#define THEBE_FREE_CAM_MIN_MOVE_SPEED		0.1

namespace Thebe
{
	class Camera;

	/**
	 * An instance of this class let's you fly around the scene.
	 */
	class THEBE_API FreeCam : public XBoxControllerButtonHandler
	{
	public:
		FreeCam();
		virtual ~FreeCam();

		void SetCamera(Camera* camera);
		Camera* GetCamera();

		void Update(double deltaTimeSeconds);

		virtual void OnButtonPressed(DWORD button) override;
		virtual void OnButtonReleased(DWORD button) override;

	private:
		Reference<Camera> camera;
		XBoxController controller;

		enum StrafeMode
		{
			XZ_PLANE,
			XY_PLANE
		};

		StrafeMode strafeMode;

		double moveSpeed;
		double rotationSpeed;
	};
}