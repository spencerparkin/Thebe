#pragma once

#include "Thebe/Common.h"
#include <Windows.h>

namespace Thebe
{
	/**
	 * This class is provided for convenience, but is not at all necessary in
	 * order to make use of the Thebe graphics engine.  All it does is provide
	 * a simple C++-style framework for creating a single-windowed win32 application.
	 * You can use any windowing framework you like (e.g., wxWidgets or Qt.)  All
	 * that Thebe requires of you is a window handle.
	 */
	class THEBE_API Application
	{
	public:
		Application();
		virtual ~Application();

		virtual bool Setup(HINSTANCE instance, int cmdShow, int width, int height);
		virtual void Shutdown(HINSTANCE instance);
		virtual int Run();

	protected:
		static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		virtual bool PrepareForWindowShow();
		virtual const char* GetWindowTitle();

		virtual LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnSize(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnDestroy(WPARAM wPara, LPARAM lParam);

		HWND windowHandle;
	};
}