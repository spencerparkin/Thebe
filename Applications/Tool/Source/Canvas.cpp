#include "Canvas.h"
#include "App.h"

GraphicsToolCanvas::GraphicsToolCanvas(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	this->Bind(wxEVT_PAINT, &GraphicsToolCanvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &GraphicsToolCanvas::OnSize, this);
}

/*virtual*/ GraphicsToolCanvas::~GraphicsToolCanvas()
{
}

void GraphicsToolCanvas::OnPaint(wxPaintEvent& event)
{
	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->Render();
}

void GraphicsToolCanvas::OnSize(wxSizeEvent& event)
{
	wxSize windowSize = event.GetSize();
	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->Resize(windowSize.x, windowSize.y);
}