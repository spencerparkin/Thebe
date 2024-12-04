#pragma once

#include "Thebe/Common.h"
#include "Thebe/Math/Vector2.h"
#include <Windows.h>
#include <Xinput.h>

namespace Thebe
{
	class XBoxControllerButtonHandler;

	/**
	 * Provide an interface to an X-box controller.
	 */
	class THEBE_API XBoxController
	{
	public:
		XBoxController(DWORD controllerNumber);
		virtual ~XBoxController();

		void Update();
		double GetTrigger(DWORD button);
		Vector2 GetAnalogJoyStick(DWORD button);
		bool WasButtonPressed(DWORD button);
		bool WasButtonReleased(DWORD button);
		bool IsButtonDown(DWORD button);
		bool IsButtonUp(DWORD button);

		// Don't let this class hang on to a stale pointer.
		void AddButtonHandler(XBoxControllerButtonHandler* buttonHandler);
		void RemoveButtonHandler(XBoxControllerButtonHandler* buttonHandler);
		void RemoveAllButtonHandlers();

	private:
		const XINPUT_STATE* GetCurrentState();
		const XINPUT_STATE* GetPreviousState();

		XINPUT_STATE stateBuffer[2];
		DWORD controllerNumber;
		UINT64 updateCount;

		std::list<XBoxControllerButtonHandler*> buttonHandlerList;
	};

	/**
	 * It can sometimes be more reliable to have the controller
	 * tell you when a button is pressed or released.
	 */
	class THEBE_API XBoxControllerButtonHandler
	{
	public:
		XBoxControllerButtonHandler();
		virtual ~XBoxControllerButtonHandler();

		virtual void OnButtonPressed(DWORD button);
		virtual void OnButtonReleased(DWORD button);
		virtual void OnButtonDown(DWORD button);
		virtual void OnButtonUp(DWORD button);
	};
}