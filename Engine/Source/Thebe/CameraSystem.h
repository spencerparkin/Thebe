#pragma once

#include "Thebe/EngineParts/Camera.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/XBoxController.h"
#include <unordered_map>

#define THEBE_FREE_CAM_MAX_MOVE_SPEED		100.0
#define THEBE_FREE_CAM_MIN_MOVE_SPEED		0.1

namespace Thebe
{
	class CameraController;

	/**
	 * Here we try to manage all the ways we can control the camera.
	 */
	class THEBE_API CameraSystem
	{
	public:
		CameraSystem();
		virtual ~CameraSystem();

		void Update(double deltaTimeSeconds);

		void AddController(const std::string& name, CameraController* controller);
		void RemoveController(const std::string& name);
		void ClearAllControllers();

		void SetActiveController(const std::string& name);
		std::string GetActiveController();
		void PushActiveController(const std::string& name);
		void PopActiveController();

		CameraController* GetControllerByName(const std::string& name);

		void SetCamera(Camera* camera);
		Camera* GetCamera();

	private:
		Reference<Camera> camera;
		double translationalBlendRate;
		double rotationalBlendRate;
		bool boundToController;
		bool doBlending;
		std::vector<std::string> activeControllerStack;
		std::unordered_map<std::string, Reference<CameraController>> controllerMap;
	};

	/**
	 * Instances of this class, as the name suggests, control the position and orientation of
	 * the camera in world space.  They don't interact with @ref Camera class instances directly,
	 * because the camera system does that and handles blending between camera controllers.
	 */
	class THEBE_API CameraController : public ReferenceCounted
	{
	public:
		CameraController();
		virtual ~CameraController();

		/**
		 * This is called each frame to place the camera in the world.
		 * 
		 * @param[in,out] cameraToWorld This is given with the camera's current camera-to-world transform and is expected to be modified by the method.
		 * @param[in] deltaTimeSeconds This is the amount of time that has passed (in seconds) since the last frame.
		 */
		virtual void ControlCamera(Transform& cameraToWorld, double deltaTimeSeconds);
	};

	/**
	 * This controller is designed to control the camera as a function of an XBoxController.
	 */
	class THEBE_API FreeCam : public CameraController, public XBoxControllerButtonHandler
	{
	public:
		FreeCam();
		virtual ~FreeCam();

		void SetXBoxController(XBoxController* controller);

		virtual void ControlCamera(Transform& cameraToWorld, double deltaTimeSeconds) override;

		virtual void OnButtonPressed(DWORD button) override;

	protected:
		Reference<XBoxController> controller;

		enum StrafeMode
		{
			XZ_PLANE,
			XY_PLANE
		};

		StrafeMode strafeMode;

		double moveSpeed;
		double rotationSpeed;
	};

	/**
	 * This controller is designed to follow a render object through space.
	 */
	class THEBE_API FollowCam : public CameraController
	{
	public:
		FollowCam();
		virtual ~FollowCam();

		virtual void ControlCamera(Transform& cameraToWorld, double deltaTimeSeconds) override;

		void SetSubject(Space* subject);
		void SetFollowDistance(double followDistance);
		double GetFollowDistance() const;

	protected:
		double followDistance;
		RefHandle subjectHandle;
	};
}