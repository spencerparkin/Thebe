#include "Application.h"
#include "Log.h"

using namespace Thebe;

Application::Application()
{
	this->windowHandle = NULL;
}

/*virtual*/ Application::~Application()
{
}

/*virtual*/ bool Application::Setup(HINSTANCE instance, int cmdShow, int width, int height)
{
	if (this->windowHandle != NULL)
		return false;

	WNDCLASSEX windowClass{};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &Application::WindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = "ThebeApplication";
	ATOM atom = RegisterClassEx(&windowClass);
	if (atom == 0)
	{
		THEBE_LOG("Failed to register window class.  Error: 0x%08x", GetLastError());
		return false;
	}

	RECT windowRect{ 0, 0, width, height };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, TRUE);

	this->windowHandle = CreateWindowA(
		windowClass.lpszClassName,
		this->GetWindowTitle(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		instance,
		this);
	if (this->windowHandle == NULL)
	{
		THEBE_LOG("Failed to create window.  Error: 0x%08x", GetLastError());
		return false;
	}

	if (!this->PrepareForWindowShow())
		return false;

	ShowWindow(this->windowHandle, cmdShow);
	return true;
}

/*virtual*/ bool Application::PrepareForWindowShow()
{
	return true;
}

/*virtual*/ const char* Application::GetWindowTitle()
{
	return "Thebe";
}

/*virtual*/ void Application::Shutdown(HINSTANCE instance)
{
}

/*virtual*/ int Application::Run()
{
	MSG msg{};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		InvalidateRect(this->windowHandle, NULL, FALSE);
		UpdateWindow(this->windowHandle);
	}

	return msg.wParam;
}

/*static*/ LRESULT Application::WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto app = reinterpret_cast<Application*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

	switch (message)
	{
		case WM_CREATE:
		{
			auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));

			app = reinterpret_cast<Application*>(createStruct->lpCreateParams);
			if (app)
				return app->OnCreate(wParam, lParam);

			break;
		}
		case WM_PAINT:
		{
			if (app)
				return app->OnPaint(wParam, lParam);

			break;
		}
		case WM_SIZE:
		{
			if (app)
				return app->OnSize(wParam, lParam);

			break;
		}
		case WM_DESTROY:
		{
			if (app)
				return app->OnDestroy(wParam, lParam);

			break;
		}
	}
	
	return DefWindowProc(windowHandle, message, wParam, lParam);
}

/*virtual*/ LRESULT Application::OnCreate(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*virtual*/ LRESULT Application::OnPaint(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*virtual*/ LRESULT Application::OnSize(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*virtual*/ LRESULT Application::OnDestroy(WPARAM wPara, LPARAM lParam)
{
	PostQuitMessage(0);
	return 0;
}