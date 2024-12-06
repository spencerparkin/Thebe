#include "App.h"
#include "Frame.h"
#include "Canvas.h"
#include <wx/image.h>

wxIMPLEMENT_APP(GraphicsToolApp);

GraphicsToolApp::GraphicsToolApp()
{
	this->frame = nullptr;
	this->graphicsEngine.Set(new Thebe::GraphicsEngine());
}

/*virtual*/ GraphicsToolApp::~GraphicsToolApp()
{
}

Thebe::GraphicsEngine* GraphicsToolApp::GetGraphicsEngine()
{
	return this->graphicsEngine.Get();
}

/*virtual*/ bool GraphicsToolApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	wxInitAllImageHandlers();

	this->frame = new GraphicsToolFrame(wxPoint(10, 10), wxSize(1200, 800));

	HWND windowHandle = this->frame->GetCanvas()->GetHWND();
	if (!this->graphicsEngine->Setup(windowHandle))
		return false;

	this->frame->Show();

	return true;
}

/*virtual*/ int GraphicsToolApp::OnExit(void)
{
	this->graphicsEngine->Shutdown();
	this->graphicsEngine = nullptr;
	return 0;
}